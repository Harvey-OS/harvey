# How to contribute

I'm really glad you're reading this, because we need volunteer developers to help this project come to fruition.

If you don't have anything you are working on we have a [**list of newbie friendly issues**](https://github.com/Harvey-OS/harvey/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22+label%3A%22newbie+friendly%22) you can help out with.

If you haven't already, come find us on our [mailing list](https://groups.google.com/forum/#!forum/harvey). We want you working on things you're excited about.

Harvey, like most other open source projects, has a [Code of Conduct](https://github.com/Harvey-OS/harvey/wiki/Code-of-Conduct) that it expects its contributors and core team members to adhere to.

Here are some important resources:

  * [Mailing list](https://groups.google.com/forum/#!forum/harvey) is where the development discussion happens.
  * [Man Pages](https://sevki.io/harvey/sys/man/1/0intro) are where you can read man pages from head of the repo.
  * [Issue Tracker](https://github.com/Harvey-OS/harvey/issues) is our day-to-day project management space.


## Testing

We make sure Travis-CI can build your pull-requests before we can accept your contributions.

## Submitting a patch

Harvey uses Github Pull Requests to accept contributions.

1.  Clone the repo `git clone https://github.com/Harvey-OS/harvey.git`. It is also possible to use git instead of https when you have an SSH public key stored in Github. Then you clone the repo with `git clone git@github.com:Harvey-OS/harvey.git`. This makes later submissions easier. For the rest of this manual we assume to use https.
2.  Check out a feature branch you will work on by `git checkout -b feature-name` for instance for issue #70, @keedon selected the branch name `statscrash`.
3.  Make changes
4.  Commit with a descriptive message, like so
    ```
    $ git commit -m "A brief summary of the commit
    >
    > A paragraph describing what changed and its impact."
    ```
    take a look at @keedon's [commit message](https://github.com/keedon/harvey/commit/09fe3a21fa8b42088bc8ad83287928e9e7cc96ef) for the same issue for a good example of a descriptive commit message.
    You can also use graphical git tools such as `git gui` which make commiting changes easier.
5.  Fork the repo (only once).
    ![harvey-os_harvey__a_fresh_take_on_plan_9](https://cloud.githubusercontent.com/assets/429977/13457174/099fb5cc-e067-11e5-83ce-f65aa966a4a9.png)
6.  Add the repo as a remote (every time you clone the repo)

    `git remote add yourname https://github.com/yourusername/harvey.git`

    where "yourname" is the name of your github login.

    `git remote -v` should look like this:
    ```
    $ git remote -v
    yourname https://github.com/yourname/harvey.git (fetch)
    yourname https://github.com/yourname/harvey.git (push)
    origin https://github.com/Harvey-OS/harvey.git (fetch)
    origin https://github.com/Harvey-OS/harvey.git (push)
    ```
7.  Push your branch to your forked repo like so `git push yourname feature-name`
8.  Create a pull request by going to `https://github.com/yourname/harvey/pull/new/feature-name` or clicking the PR button
    ![sevki_harvey__a_64_bit_operating_system_based_on_plan_9_from_bell_labs_and_nix__under_gpl](https://cloud.githubusercontent.com/assets/429977/13457635/79359350-e069-11e5-987b-1b4fccc45372.png)
9.  Add details of what you have worked on and what your intentions are.
    ![comparing_harvey-os_master___sevki_travis-trials_ _harvey-os_harvey](https://cloud.githubusercontent.com/assets/429977/13457683/aa2a423a-e069-11e5-84cc-1173e33264cb.png)
    When you send a pull request, we will love you forever if you include qemu outputs and more tests. We can always use more test coverage. Please follow our coding conventions (below) and make sure all of your commits are atomic (one feature per commit).

### Iterating on your work

- A Github PR or a Gerrit CL roughly serve the same purpose, they are feedback loops for people to give feedback and for you to iterate on your work, so be ready to repeat steps 3, 4 and 7.
- When iterating on your work continue committing to the same branch and push the changes up to your fork. Github will track the changes and update the PR accordingly.
- Most of the time while you have submitted your PR, someone else will have merged something to master, at which point your branch will have been outdated, GitHub provides a convenient way of updating your branch right from your pull request.
![build_files_by_sevki_ _pull_request__53_ _harvey-os_harvey](https://cloud.githubusercontent.com/assets/429977/13457994/4d9a3118-e06b-11e5-9898-f8574b5ce11d.png)
- When you click that button, GitHub will update your branch, at which point you will have to `git pull yourname feature-name` and update your local repo before committing more changes.
- If your changes are conflicting with something that was merged into the branch, you will have to resolve the changes manually and submit the changes.

### Keeping up to date with the master branch

If you're working in a branch that is outdated with respect to the master branch, just do a `git pull --rebase`. This will put your changes after the pull. In the case that there would be conflicts, you will have to solve them manually, but they are marked with something like ">>>>>HEAD" and git will tell you about which files are conflicting.

#### More information

- [How to use Pull Requests](http://help.github.com/pull-requests/)
- [GitHub Flow](https://guides.github.com/introduction/flow/)
- [Hub: A command line application for GH Flow](https://hub.github.com)
- [Video: Pull Requests](https://www.youtube.com/watch?v=81uKcXZoQ2A)
- [Video: Workflows](https://www.youtube.com/watch?v=EwWZbyjDs9c)
- [Github Desktop Client](https://desktop.github.com/)
- [Video: Undo, Redo & Rebase Your Git History](https://www.youtube.com/watch?v=W39CfI3-JFc)
- [Video: Git Rebase](https://www.youtube.com/watch?v=SxzjZtJwOgo)


## Coding conventions

If you read the code you should get a hang of it but a loose listing of our [Style-Guide](https://github.com/Harvey-OS/harvey/wiki/Style-Guide) exists, we recommend you check it out.

We have also automated the process via [clang-format](http://clang.llvm.org/docs/ClangFormat.html) so before you submit a change please format your diff.

[How to clang-format a diff](http://clang.llvm.org/docs/ClangFormat.html#script-for-patch-reformatting)


_Adopted from [Open Government Contribution Guidelines](https://github.com/opengovernment/opengovernment/blob/master/CONTRIBUTING.md)_
