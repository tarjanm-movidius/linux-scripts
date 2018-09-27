# /etc/skel/.bashrc
#
# This file is sourced by all *interactive* bash shells on startup,
# including some apparently interactive shells such as scp and rcp
# that can't tolerate any output.  So make sure this doesn't display
# anything or bad things will happen !

# Test for an interactive shell.  There is no need to set anything
# past this point for scp and rcp, and it's important to refrain from
# outputting anything in those cases.
if [[ $- != *i* ]]; then
	# Shell is non-interactive.  Be done now!
	return
fi

# Put your fun stuff here.

alias xcdroast='xcdroast -n'
alias mc='grep -q '\'' 1;0;0$'\'' ~/.local/share/mc/filepos && mv ~/.local/share/mc/filepos ~/.local/share/mc/filepos~ && grep -v '\'' 1;0;0$'\'' ~/.local/share/mc/filepos~ > ~/.local/share/mc/filepos; mc'

if [ -x /usr/bin/fortune ]; then
  /usr/bin/fortune
  echo
fi

if [ -x /usr/bin/nevnap ]; then
  /usr/bin/nevnap
  echo
fi
