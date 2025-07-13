Contributing to the Dirk Engine!
================================

Thanks for your interest in contributing to the Dirk Engine!

## Language

Although I do speak French, please keep this codebase in English.

## Reporting Bugs

Bugs should be reported on the [GitHub Issue Tracker][issue-tracker] using the bug report template.

Follow the advice in [How do I ask a good question?][how-to-ask]. While the article is intended for people asking questions on Stack Overflow, it all applies to writing a good bug report too.

## Requesting New Features

Feature requests should also be sent to the [GitHub Issue Tracker][issue-tracker] using the feature request template.

- Explain what the new feature is and why it should be added.

- If possible, provide an example, like a code snippet, showing what your new feature might look like in use.

Also, much of the advice in [How do I ask a good question?][how-to-ask] applies here too.

## Contributing a Fix or Feature

1. If you haven't already, create a fork of the repository.

2. Create a branch, and make all of your changes on that branch.

3. Submit a pull request and fill in the template.

4. Wait for a review. This project is just a hobby that I work on in my free time so it may take a while for me to review the PR.

If you're not sure what any of that means, check out Thinkful's [GitHub Pull Request Tutorial][thinkful-pr-tutorial] for a complete walkthrough of the process.

### Writing a Good Pull Request

- Stay focused on a single fix or feature. If you submit multiple changes in a single request, the rejection of one change will lead to the rejection of the entire PR. If you submit each change in its own request it is easier for us to review and approve (especially since the PRs don't get very big).

- Limit your changes to only what is required to implement the fix or feature. In particular, avoid style or formatting tools that may modify the formatting of other areas of the code.

- Use descriptive commit titles/messages. "Implemented \<feature\>" or "Fixed \<problem\> is better than "Updated \<file\>".

- Make sure the code you submit compiles and runs without issues. When we set up unit tests and continuous integration we also expect that the pull request should pass all tests.

- Use [closing keywords][github-help-closing-keywords] in the appropriate section of our Pull Request template where applicable.

- Follow our coding conventions, which we've intentionally kept quite minimal.

### Coding Conventions

- Naming convention:
  - For functions we use pascal case: **`FunctionName`**.
  - For variables, class members, and function parameters we use camel case: **`variableName`** and **`parameterName`**.

  - For class names we use pascal case: **`ClassName`**.

  - For macros we use shouting snake case: **`MACRO_NAME`**.
    - If it is specifically related to the engine, we add the 'DIRK_' prefix: **`HZ_MACRO_NAME`**.

- Use spaces for indentation, not tabs.

- Don't forget to follow the rules in **`.clang-format`**

- When in doubt, match the code that's already there.

[github]: https://github.com
[how-to-ask]: https://stackoverflow.com/help/how-to-ask
[issue-tracker]: https://github.com/PiquelChips/DirkEngine/issues
[submit-pr]: https://github.com/PiquelChips/DirkEngine/pulls
[thinkful-pr-tutorial]: https://www.thinkful.com/learn/github-pull-request-tutorial/
[github-help-closing-keywords]: https://help.github.com/en/articles/closing-issues-using-keywords
