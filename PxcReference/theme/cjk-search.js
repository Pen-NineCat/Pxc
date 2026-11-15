// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat
/**
 * @brief 让 mdBook 自带的搜索认得中文：接管索引加载，改成按字匹配。
 *
 * mdBook 在构建时就把搜索索引建好了（`window.search.index` 是一份 elasticlunr 索引的
 * 序列化结果），分词只认空白与标点：一整段中文会被当成一个词，甚至被 trimmer 直接丢掉
 * ——实测这本书的索引里，30 节正文只有 1 个含中文的 token，所以「存储期」搜不到
 * 「三种存储期」。mdBook 没有暴露分词器配置，但索引里每节原文完整留在
 * `documentStore.docs`（title / body / breadcrumbs），而搜索器对索引的全部用法只有
 * 一句 `elasticlunr.Index.load(index).search(词, 选项)`。所以这里把 `Index.load`
 * 换掉，返回一个只实现 `search` 的对象：分词、打分、摘要全在浏览器里做，
 * 不必改 mdBook，也不必改构建产物或加构建期依赖。删掉 book.toml 里
 * `additional-js` 的这一项，就退回 mdBook 自带的（不认中文的）搜索。
 *
 * 切词规则：中文出相邻两字（单字查询就是那个字），英文出整个单词；
 * 查询里的每个词都必须出现在同一节里（与 [output.html.search] 的 use-boolean-and 一致）；
 * 分数按字段加权，标题最重；与当前页面同语言的结果再翻一倍——中英两版内容成对，
 * 不加这一下，在中文页搜 `ReflectState` 会先看到英文那份。
 *
 * 另外返回的 `doc.body` 换成命中处附近的一小段——搜索器的摘要按空格数取词，
 * 中文整节只有一个「词」，不裁的话每条结果会把整节正文都印出来。
 */
(function () {
    "use strict";

    var elasticlunr = window.elasticlunr;
    if (!elasticlunr || !elasticlunr.Index) {
        return;
    }

    // 字段权重：标题命中比正文命中更值钱
    var FIELD_WEIGHT = { title: 4, breadcrumbs: 1, body: 1 };
    // 同语言加成：只是偏好，不是过滤（英文页搜英文词时本来就只有英文那份命中）
    var SAME_LANGUAGE_BOOST = 2;
    var CJK = "\\u3400-\\u4dbf\\u4e00-\\u9fff\\uf900-\\ufaff";
    var CJK_RUN = new RegExp("[" + CJK + "]+", "g");
    var NON_CJK_RUN = new RegExp("[^" + CJK + "]+", "g");
    // 摘要窗口：命中位置前后各留多少字
    var SNIPPET_BEFORE = 24;
    var SNIPPET_AFTER = 80;

    // 当前页面在哪门语言下：语言目录从 doc_urls 的第一段推出来（不在这里写死 zh/en），
    // 再在 pathname 里找 `/<语言>/`。索引要等第一次搜索才下载，所以这件事第一次搜索时再算。
    var pageLanguage = null;
    function currentLanguage() {
        if (pageLanguage !== null) {
            return pageLanguage;
        }
        pageLanguage = "";
        var docUrls = (window.search && window.search.doc_urls) || [];
        var languages = {};
        var path = window.location.pathname;

        for (var i = 0; i < docUrls.length; i++) {
            var language = String(docUrls[i]).split("/")[0];
            if (language !== "") {
                languages[language] = true;
            }
        }
        for (var candidate in languages) {
            if (Object.prototype.hasOwnProperty.call(languages, candidate)
                && path.indexOf("/" + candidate + "/") !== -1) {
                pageLanguage = candidate;
                return pageLanguage;
            }
        }
        return pageLanguage;
    }

    // 查询切成「必须同时出现」的若干词：中文按相邻两字切，英文按空白与标点切
    function queryTerms(query) {
        var lower = query.toLowerCase();
        var terms = [];
        var i;

        var cjkRuns = lower.match(CJK_RUN) || [];
        for (i = 0; i < cjkRuns.length; i++) {
            var run = cjkRuns[i];
            if (run.length === 1) {
                terms.push(run);
                continue;
            }
            for (var at = 0; at + 1 < run.length; at++) {
                terms.push(run.slice(at, at + 2));
            }
        }

        // 注意：每段 NON_CJK_RUN 是「一整段非中文」，空格与标点也在里面，必须按它们切开。
        // 若把分隔符删掉，"read this book" 会拼成 readthisbook，scoreDoc 要求整串字面出现，英文查询就全落空。
        var latinRuns = lower.match(NON_CJK_RUN) || [];
        for (i = 0; i < latinRuns.length; i++) {
            var words = latinRuns[i].match(/[0-9a-z]+/g) || [];
            for (var w = 0; w < words.length; w++) {
                terms.push(words[w]);
            }
        }

        // 同一个词出现两次不必要求两遍
        return terms.filter(function (term, index) {
            return terms.indexOf(term) === index;
        });
    }

    function countOccurrences(haystack, needle) {
        var count = 0;
        var at = haystack.indexOf(needle);
        while (at !== -1) {
            count++;
            at = haystack.indexOf(needle, at + needle.length);
        }
        return count;
    }

    // 每个查询词都得在这一节里出现，否则不成结果；出现得越多、字段越重要分数越高
    function scoreDoc(doc, terms) {
        var score = 0;
        for (var i = 0; i < terms.length; i++) {
            var term = terms[i];
            var hit = 0;
            for (var field in FIELD_WEIGHT) {
                if (Object.prototype.hasOwnProperty.call(FIELD_WEIGHT, field)) {
                    hit += countOccurrences((doc[field] || "").toLowerCase(), term) * FIELD_WEIGHT[field];
                }
            }
            if (hit === 0) {
                return 0;
            }
            score += hit;
        }
        return score;
    }

    // 命中处附近的一小段，太长/太靠后的正文不进结果行
    function makeSnippet(body, terms) {
        var lower = body.toLowerCase();
        var best = -1;
        for (var i = 0; i < terms.length; i++) {
            var at = lower.indexOf(terms[i]);
            if (at !== -1 && (best === -1 || at < best)) {
                best = at;
            }
        }
        if (best === -1) {
            return body.slice(0, SNIPPET_BEFORE + SNIPPET_AFTER) + "…";
        }
        var start = Math.max(0, best - SNIPPET_BEFORE);
        var end = Math.min(body.length, best + SNIPPET_AFTER);
        return (start > 0 ? "…" : "") + body.slice(start, end) + (end < body.length ? "…" : "");
    }

    function search(docs, query) {
        var terms = queryTerms(query);
        if (terms.length === 0) {
            return [];
        }

        var docUrls = (window.search && window.search.doc_urls) || [];
        var language = currentLanguage();
        var hits = [];

        for (var ref in docs) {
            if (!Object.prototype.hasOwnProperty.call(docs, ref)) {
                continue;
            }
            var doc = docs[ref];
            var score = scoreDoc(doc, terms);
            if (score === 0) {
                continue;
            }
            if (language !== "" && String(docUrls[ref] || "").split("/")[0] === language) {
                score *= SAME_LANGUAGE_BOOST;
            }
            hits.push({
                ref: ref,
                score: score,
                doc: {
                    id: doc.id,
                    title: doc.title,
                    breadcrumbs: doc.breadcrumbs,
                    body: makeSnippet(doc.body || "", terms),
                },
            });
        }

        hits.sort(function (a, b) {
            return b.score - a.score;
        });
        return hits;
    }

    var load = elasticlunr.Index.load;
    elasticlunr.Index.load = function (index) {
        var docs = index && index.documentStore && index.documentStore.docs;
        if (!docs) {
            // 索引里没有原文（更老或更怪的 mdBook）时退回自带行为
            return load.call(elasticlunr.Index, index);
        }
        return {
            search: function (query) {
                return search(docs, query);
            },
        };
    };
})();
