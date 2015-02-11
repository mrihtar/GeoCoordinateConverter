BEGIN {
  n = 0;
}{
  if (FNR == 1) next;
  sub("[[:space:]]*$", "");
  col1[n] = $1;
  y[n] = $4;
  x[n] = $5;
  n++;
} END {
  print n, "2 0 0";
  for (ii = 0; ii < n; ii++) {
    print ii, y[ii], x[ii]
  }
}
