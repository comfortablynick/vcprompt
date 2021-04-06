bin_name := 'vctest'

alias run   := file-run
alias r     := file-run
alias build := file-build
alias b     := file-build
alias rb    := run-binary
alias i     := install

clean:
    rm -rf build

file-build:
    xmake build

install: file-build
    xmake install

@file-run: file-build
    xmake run
             
run-binary:
    ./build/linux/x86_64/release/{{bin_name}}

# run valgrind to check for memory leaks
check:
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             ./build/linux/x86_64/release/{{bin_name}}
