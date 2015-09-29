@echo off


IF NOT EXIST libserg goto ls_clone
pushd libserg
git pull
popd
goto ls_done

:ls_clone
git clone https://github.com/serge-rgb/libserg

:ls_done

