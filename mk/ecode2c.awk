BEGIN {
  idx=0
  printf "static const char *const s_error_strs[] = {\n"
}

{
  printf "\t\"%s\",\t/* %3d */\n", $2, idx++
}

END {
  printf "\n\tNULL\n"
  printf "};\n"
}
