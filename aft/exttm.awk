BEGIN {
  n = 0;
}{
  if (FNR == 1) next;
  sub("[[:space:]]*$", "");
  col1[n] = $1;
  y[n] = $2;
  x[n] = $3;
  n++;
} END {
  print n, "2 0 0";
  for (ii = 0; ii < n; ii++) {
    print ii, y[ii], x[ii]
  }
}
