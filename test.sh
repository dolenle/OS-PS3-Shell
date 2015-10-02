#!./a.out
rm -f hello
rm -f world
echo hello >hello
echo world >world
echo stupid >stupid
ls -l >out
cat hello - world >>out <stupid
