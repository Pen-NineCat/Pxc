// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat
/**
 * @brief 逐条判断规则标签（正文左栏的 `[storage.kinds]`）放不放得下，放不下的换成 `[*]`。
 *
 * 左栏宽度由 CSS 栅格的 `1fr` 决定，而「这条 ID 能不能在一行里排完」只有排版之后才知道。
 * CSS 自己没有测量文本溢出的能力：`container-type` 一旦加在元素上，该元素的尺寸就与内容
 * 无关了（这正是 pxc.css 里那个容器查询只能按「栏宽」整页判断的原因）。所以：
 *
 *   1. `.rule-link` 在 CSS 里是 `white-space: nowrap` + `overflow: hidden`，且
 *      `<wbr>` 被 `display: none` 关掉，所以标签只会是一条不折行的直线；
 *   2. 这里比较标签自身的排版宽度与链接的内容盒宽度（不能用 `scrollWidth`：标签右对齐，
 *      超长时溢出落在左侧，`scrollWidth` 量不到）；
 *   3. 放不下的给 `.rule` 加 `.rule--tight`，由 CSS 把它换成 `[*]`（与「整栏太窄」时
 *      的容器查询共用同一套选择器，两态外观一致）。
 *
 * 排版的输入有三条来源，三条都要盯：窗口尺寸变化、字体加载完成，以及 mdBook 自己
 * 改栏宽——侧栏开关只改 `.page-wrapper` 的 margin、拖侧栏只改侧栏宽度，这两条都
 * 不派发 `window.resize`，所以要额外用 ResizeObserver 盯 `main` 的宽度。
 * 没有这段脚本时标签只是被裁掉（不会折行、不会压到「上一页」箭头上）。
 */
(function () {
    "use strict";

    // 全书合一页（print.html）没有箭头占位、页面宽度也和正文页不同，
    // 在那里逐条量出来的结论没有意义，整页都会变 [*]。
    if (location.pathname.endsWith("/print.html")) {
        return;
    }

    function fit() {
        document.querySelectorAll("main > .rule > .rule-link").forEach(function (link) {
            var rule = link.parentElement;
            // 先复位再量：带着上一轮的 [*] 量，标签宽度恒为 0，窗口变宽之后
            // 标签就再也回不来了。
            rule.classList.remove("rule--tight");

            var label = link.firstElementChild;
            if (!label) {
                return;
            }
            // 不能用 scrollWidth 判断：标签是右对齐的，超长时溢出发生在**左侧**，
            // scrollWidth 只统计行尾方向的溢出，量不到。改成比较标签自身的排版宽度
            // 与链接的内容盒宽度（getBoundingClientRect 会给出被裁掉之前的真实宽度）。
            var style = getComputedStyle(link);
            var avail =
                link.clientWidth -
                parseFloat(style.paddingLeft) -
                parseFloat(style.paddingRight);
            if (label.getBoundingClientRect().width > avail + 0.5) {
                rule.classList.add("rule--tight");
            }
        });
    }

    var pending = false;
    function schedule() {
        if (pending) {
            return;
        }
        pending = true;
        requestAnimationFrame(function () {
            pending = false;
            fit();
        });
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", fit);
    } else {
        fit();
    }

    window.addEventListener("resize", schedule);
    if (document.fonts && document.fonts.ready) {
        document.fonts.ready.then(schedule);
    }

    // 侧栏开关只改 .page-wrapper 的 margin-inline-start（还带 0.3s 过渡），拖侧栏只改
    // 侧栏宽度，两条路径都不派发 window.resize，但 main 的栅格宽度会变——不盯住它们，
    // 收起侧栏后本来排得下的标签会一直停在 [*]，展开后则反过来继续溢出。
    // fit() 只切换 display，不改变 main 的宽度，所以这个观察者不会再把自己触发一轮。
    if (typeof ResizeObserver === "function") {
        var main = document.querySelector("main");
        if (main) {
            var lastWidth = -1;
            new ResizeObserver(function (entries) {
                var width = entries[0].contentRect.width;
                if (width === lastWidth) {
                    return;
                }
                lastWidth = width;
                schedule();
            }).observe(main);
        }
    }
})();
