gitconfig:
	for hook in commit-msg ; do if ! [ -x .git/hooks/$$hook ]; then cp util/gitconfig/$$hook .git/hooks/$$hook; chmod +x .git/hooks/$$hook; fi; done
	git config remote.origin.push HEAD:refs/for/master
	git config branch.autosetupmerge always
	echo "Don't forget to set up your name and email in git"
