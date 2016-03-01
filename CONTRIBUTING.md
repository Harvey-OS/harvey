# How to contribute

I'm really glad you're reading this, because we need volunteer developers to help this project come to fruition.

If you haven't already, come find us on our [mailing list](https://groups.google.com/forum/#!forum/harvey). We want you working on things you're excited about.

Harvey, like most other open source projects, has a [Code of Conduct](https://github.com/Harvey-OS/harvey/wiki/Code-of-Conduct) that it expects its contributors and core team members to adhere to.

Here are some important resources:

  * [Mailing list](https://groups.google.com/forum/#!forum/harvey) is where the development discussion happens.
  * [Man Pages](https://sevki.io/harvey/sys/man/1/0intro) are where you can read man pages from head of the repo.
  * [Issue Tracker](https://github.com/Harvey-OS/harvey/issues) is our day-to-day project management space.


## Testing

 We make sure Travis-CI can build your pull-requests before we can accept your contributions.

## Submitting changes

Please send a [Pull Request to Harvey OS](https://github.com/Harvey-OS/harvey/pull/new/master) with a clear list of what you've done (read more about [pull requests](http://help.github.com/pull-requests/) and [Github Flow](https://guides.github.com/introduction/flow/)).

Some of our contributors like using [hub](http://hub.github.com) for sending pull requests from the command line, an example workflow for contributing to harvey would look like (after you've [installed hub](http://hub.github.com))
``` 
$ git clone git@github.com:Harvey-OS/harvey.git
$ cd harvey
```
create a topic branch
``` 
$ git checkout -b feature
  ( making changes ... )
$ git commit -m "some descriptive thing about what you have done"
``` 
It's time to fork the repo!
``` 
$ hub fork
→ (forking repo on GitHub...)
→ git remote add YOUR_USER git://github.com/YOUR_USER/harvey.git
```
push the changes to your new remote
``` 
$ git push YOUR_USER feature
```
open a pull request for the topic branch you've just pushed
``` 
$ hub pull-request
→ (opens a text editor for your pull request message)
```
and someone from the core team will review it. 

When you send a pull request, we will love you forever if you include qemu outputs and more tests. We can always use more test coverage. Please follow our coding conventions (below) and make sure all of your commits are atomic (one feature per commit).

Always write a clear log message for your commits. One-line messages are fine for small changes, but bigger changes should look like this:

    $ git commit -m "A brief summary of the commit
    >
    > A paragraph describing what changed and its impact."

## Coding conventions

If you read the code you should get a hang of it but a loose listing of our [Style-Guide](https://github.com/Harvey-OS/harvey/wiki/Style-Guide) exists, we recommend you check it out.

We have also automated the process via [clang-format](http://clang.llvm.org/docs/ClangFormat.html) so before you submit a change please format your diff.

[How to clang-format a diff](http://clang.llvm.org/docs/ClangFormat.html#script-for-patch-reformatting)


_Adopted from [Open Government Contribution Guidelines](https://github.com/opengovernment/opengovernment/blob/master/CONTRIBUTING.md)_
