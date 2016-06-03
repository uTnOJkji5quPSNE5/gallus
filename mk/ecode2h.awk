BEGIN {
  idx=0
}

{
  printf "#define %s%s\t%d\n", prfx, $1, idx--;
}

END {
  printf "\n#define %sERROR_CODE_MIN\t%d\n", prfx, ++idx
}

