@echo off
git -C "%~dp0.." config core.hooksPath tools/git-hooks
echo Git hooks installed: tools/git-hooks
