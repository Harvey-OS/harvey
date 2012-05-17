gitconfig:
	for hook in commit-msg ; do if ! [ -x .git/hooks/$$hook ]; then cp util/gitconfig/$$hook .git/hooks/$$hook; chmod +x .git/hooks/$$hook; fi; done
	echo "Don't forget to set up your name and email in git"
