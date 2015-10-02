#!./a.out
rm -f hello
rm -f world
echo hello >hello
echo world >world
ls -l >out
cat hello - world >>out
