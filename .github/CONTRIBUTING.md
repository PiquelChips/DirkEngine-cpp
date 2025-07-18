Contributing to the Dirk Engine
===============================

Thank you for your interest in contributing to the Dirk Engine!

## Language

Please keep this codebase in **proper** English. Don't be afraid to correct people's
mistakes (including mine), just be polite about it.

## Reporting Bugs

Bugs should be reported on the [GitHub Issue Tracker][issue-tracker] using the bug
report template. Follow the advice in [How do I ask a good question?][how-to-ask].
While the article is intended for people asking questions on Stack Overflow, it also
applies to writing a good bug report too.

## Requesting New Features

Feature requests should also be sent to the [GitHub Issue Tracker][issue-tracker] using the feature request template.
- Explain what the new feature is and why it should be added.
- If possible, provide an example, like a code snippet or a screenshot, showing what your new feature might look like.
Much of the advice in [How do I ask a good question?][how-to-ask] applies here too.

## Contributing a Fix or Feature

1. If you haven't already, create a
   [fork](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/about-forks)
   of the repository.
2. Create a
   [branch](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-branches),
   and make all of your changes on that branch (this is especially important as I squash PRs before merging them,
   so once the PR is merged, the branch effectively becomes useless).
3. Submit a
   [pull request](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-pull-requests)
   and fill in the template.
4. Wait for a review. This project is just a hobby that I work on in my free time so it may take a while for me
   to review your PR.

### Writing a Good Pull Request

- Stay focused on a single fix or feature. If you submit multiple changes in a single request,
  the rejection of one change will lead to the rejection of the entire PR. If you submit each
  change in its own request it is easier for us to review and approve. This also helps avoid
  PRs getting too large and thus difficult to review.
- Limit your changes to only what is required to implement the fix or feature. In particular,
  avoid style or formatting tools that may modify the formatting of other areas of the code
  (stick to this repository's
  [ClangFormat style options](https://github.com/PiquelChips/DirkEngine/blob/main/.clang-format)).
- Use descriptive commit titles/messages. "Implemented \<feature\>" or "Fixed \<problem\> is
  better than "Updated \<file\>".
- Make sure the code you submit compiles and runs without issues. When I
  [setup a CI pipeline](https://github.com/PiquelChips/DirkEngine/issues/33),
  I will expect all tests to pass before merging the PR.
- Use [closing keywords](https://help.github.com/en/articles/closing-issues-using-keywords)
  in the appropriate section of the PR template.
- Follow the [coding conventions](#coding-conventions) of the project.

### Coding Conventions

- Naming convention:
  - For functions and class names use pascal case: **`FunctionName`**.
  - For macros use shouting snake case: **`MACRO_NAME`**.
    - If it is specifically related to the engine, add the 'DIRK_' prefix: **`DIRK_MACRO_NAME`**.
  - For everything else (variables, class members, and function parameters) use camel case: **`variableName`**.
- Use 4 spaces for indentation, not tabs.
- Don't forget to follow the formatting rules in
  [**`.clang-format`**](https://github.com/PiquelChips/DirkEngine/blob/main/.clang-format)
- When in doubt, match the code that's already there.

[how-to-ask]: https://stackoverflow.com/help/how-to-ask
[issue-tracker]: https://github.com/PiquelChips/DirkEngine/issues
