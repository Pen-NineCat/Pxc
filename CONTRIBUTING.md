# Contributing

Thanks for helping improve Pxc. We welcome contributions and suggestions. Even if your contribution is not ultimately accepted, we greatly appreciate the fact that you put your valuable time and effort on Pxc.

## LLM policy

  We accept LLM-assisted contributions.

- **Disclosing it.** In your issue, PR description, and before merge, say which parts were LLM-generated — code, tests, docs, comments, commit messages — and what you did with them: taken as-is, edited, or reviewed line by line. Write this yourself, in your own words. "LLM was used" doesn't tell a reviewer which claims in the diff still need checking.

- **Writing the description yourself.** The issue and PR description reflects your intentions and thoughts about these changes. We can already generate a summary of your diff; we can't generate evidence that you read it. Tell us what you were trying to do and what you decided against. That part isn't recoverable from the code, and it's the part that makes the change yours.

- If disclosure is a problem for you, don't send the PR. We'd rather hear it before merge than after.

## Preparation

Before start your brainstorm, you can see our [Issues](https://github.com/Pen-NineCat/Pxc/issues) and [Projects](https://github.com/Pen-NineCat/Pxc/projects) page to avoid reporting duplicate.

## Bug issue

You can report Pxc component bugs here, where they'll be directly reviewed by component maintainers. Before submitting a bug issue, please help us efficiently process bug by following tips:

- You should be confident that your finding is an **actual implementation** bug, instead of undefined behavior or surprising-standard behavior.
- You should prepare a self-contained command line test case and as small as possible to help us locate the bug(error, misbehavior or other we don't want), source file, command line input and your expected output. Self-contained means that your source file has to avoid including code that we don't have.
- Adding component labels to an issue, like `PLib`,`PPak`,`PLang`,`PLangLs`,`Bug` and so on, can help component maintainers or contributors who are actually handling it to locate the issue.
- A good title is helpful. We prefer **"`<UnitName>`: Short description of your issue"**, and you don't need to mention `Std` or Pxc.

## Review a pull request

We love code reviews from contributors! Reviews from contributors can often accelerate the workflow by helping PR author to find and commit changes before we review.

To join our Code Review process, you can check out our [Code Reviews](https://github.com/users/Pen-NineCat/projects/5/) workflow.

## Submit a pull request

Before submitting a PR, please ensure that:

- Please **read our LLM policy** first.
- These changes introduce **no public known API changes**. 
- Your changes need be written from scratch using only this repository, or the Pxc Reference. If your changes are derived from a project, please **mention** it. And if it's not listed in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md), we need to determine whether the license is compatible.

## Code of Conduct

All interactions for Pxc are covered by [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md). Everyone should be friendly and respectful of others.

