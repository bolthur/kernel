# How to contribute

It's important to us that you feel you can contribute towards the evolution of bolthur/kernel. This can take many forms: from helping to fix bugs or improve the docs, to adding in new features to the source. This guide should help you in making that process as smooth as possible.

Before contributing, please read the [Code of Conduct](CODE_OF_CONDUCT.md) which is adapted from [Contributor Covenant, version 1.4](https://www.contributor-covenant.org/version/1/4).

## Reporting issues

To report a bug, request a feature or simply ask a question, make use of GiHub Issues section for [Issues](https://github.com/bolthur/kernel/issues). When submitting an issue please take care of the following stepsÖ

1. **Search for already existing issues.** Your question or bug may already have been answered or fixed. Be sure to search issues first before putting in a duplicate issue.

2. **Create an isolated and reproducible test case.** If you are reporting a bug, make sure you also have a minimal, runnable, code example that reproduces the problem you have. That makes it easier to fix something.

3. **Share as much information as possible.** Include browser version affected, your OS, version of the library, steps to reproduce as also written within issue template. Something like "X isn't working!!!1!" will probably just be closed.

## Contributing changes

### Setting up

To setup for making changes you'll need to take a few steps, which we've outlined below:

1. Fork repository [bolthur/kernel](https://github.com/bolthur/kernel). If you're not sure how to fork the repository, GitHub has a [guide](https://help.github.com/articles/fork-a-repo/) for the process and checkout the fork.

2. Setup cross compilation environment. Easiest way to achieve that is to use the shell script from repo [bolthur/workspace](https://github.com/bolthur/workspace). Annotation for mac users, having binutils installed via brew or something else may lead to errors during compilation of cross compiler.

3. Prepare checkout for compilation by executing command `autoreconf -iv` within checkout. If already checked out and executed, and some new files have been added to one of the `Makefile.am` files, you have to execute `autoreconf -iv`. Both the shell script and the autoreconf have to be executed within project root.

4. Create a build directory and execute configure there with necessary parameters.

### Making a change

Once the repository has been checked out and everything has been prepared including the cross development environment, you're almost ready to make a change. The last point to be done before you start is checking out the correct branch for the change itself. Which branch shall be used depends on the type of change you're going to do.

Short branch breakdown

- `master` - Make your change at the `master` branch in case that it is an *urgent* hotfix
- `develop` - Make your change at the `develop` branch, when it's a *non-urgent* bugfix or feature.

The change should be made directly to the correct branch within your fork or to a branch branched from the correct branch listed above. Also ensure, that there is an issue existing for the change you want to submit.

### Testing Your change

You can test your change by using a real device or something like qemu. If you're going to fix a bug, please test it against the mentioned error device, e.g. qemu or some real device.

### Submitting Your change

After you've made and tested the change, commit and push it to the fork. After that open a [draft pull request](https://github.blog/2019-02-14-introducing-draft-pull-requests/) from your fork to the main repository on the branch used in `Making a change` section of this document. Keep the fork up to date by using rebase instead of normal merge which would result in more merge commits than necessary.

Once all discussions have been completed, and the related issue is scheduled within the current milestone, the pull request is going to be merged.

## Code style guide

- Use 2 spaces for tabs, never tab characters.
- No trailing whitespace and consecutive blank lines, blank lines should have no whitespace.
- Follow conventions already in the code.

## Contributor Code of Conduct

[Code of Conduct](CODE_OF_CONDUCT.md) is adapted from [Contributor Covenant, version 1.4](http://contributor-covenant.org/version/1/4)

## Post scriptum

Thanks to the author who created the original [Pixi.js](https://github.com/pixijs/pixi.js) contributing file which we adapted for this project.
