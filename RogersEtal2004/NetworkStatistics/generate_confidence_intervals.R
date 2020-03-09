print_stats <- function(filename) {
  area <- read.table(filename, sep='\t', head=TRUE)
  m <- mean(area$Diff)
  s <- sd(area$Diff)
  n <- length(area$Diff)
  error <- qnorm(0.975)*s/sqrt(n)
#  sprintf("Filename is %s", filename)
  sprintf("Mean is: %5.3f; SD is %5.3f", m, s)
#  sprintf("95%% CI is: [%5.3f, %5.3f]", m-error, m+error)
#  sprintf("Range is: [%5.3f, %5.3f]", min(area$Diff), max(area$Diff))
}

print_stats("Patterns P2_series_zero_domain_accuracy.dat")
print_stats("Patterns P2_series_noise_domain_accuracy.dat")
print_stats("Patterns P2_series_ablate_domain_accuracy.dat")
print_stats("Patterns P2_series_scale_domain_accuracy.dat")