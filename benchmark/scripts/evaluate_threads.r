#install.packages( "ggplot2" )
library(ggplot2)
#library(plyr)
library(reshape2)

source( "scripts/evaluate_config.r" )

plot_distribution <- function( timing, ht ) {
    timing <- timing[1:48,c(-1,-2)]
    if( !ht ) timing <- timing[1:24,]


    # --------------------------
    # get distribution of values
    length = length( timing[,1] )
    no_simd_idx = seq( 3, length, 3 )
    bit_16_idx = seq( 2, length, 3 )

    reduced_timing <- timing[-bit_16_idx,]
    reduced_timing <- reduced_timing[ which( reduced_timing == "NW" ), ]

    reduced_labels <- unique( apply( reduced_timing[,1:3], 1, paste, collapse = "," ) )
    reduced_labels <- gsub( pattern = ",", replacement = ", ", x = reduced_labels)
    reduced_labels <- gsub( pattern = "_t", replacement = " threads", x = reduced_labels)
    reduced_labels <- gsub( pattern = "_", replacement = " ", x = reduced_labels)
    reduced_labels <- gsub( pattern = "8 b", replacement = "  8 b", x = reduced_labels)
    reduced_labels <- gsub( pattern = "1 threads", replacement = "1 thread  ", x = reduced_labels)

    l_reduced_labels = length(reduced_labels)

    data <- t(reduced_timing[,4:13])

    # plot graph
    if( ht ) {title <- "Runtime variation (HT activated)"} else {title <- "Runtime variation"}

    if( ht ) {filename <- add_output_dir( "runtime_variation_threads_ht" )} else {filename <- add_output_dir( "runtime_variation_threads" )}

    pdf(file=filename, width = 10, height = 6, pointsize = 12)
    par( mar = c( 5, 4.5, 3, 1 ) )
    x = boxplot( data, main = title, horizontal = F, ylab = "Time (seconds)", xlab = "Threads (count)", names = reduced_labels, las = 1, log = "y", xaxt="n" )

    abline( v = seq(0.5, (l_reduced_labels + 1), 2), col = "black", lty=2 )

    if( ht ) {labels <- c(1:8)} else {labels <- c(1:4)}

    axis( side = 1, at=seq(1.5, l_reduced_labels, 2), labels=labels )

    bit_8_idx = seq( 1, l_reduced_labels, 2 )
    bit_64_idx = seq( 2, l_reduced_labels, 2 )

    lines( bit_8_idx, apply( data[,bit_8_idx], 2, mean) , col="red", type = "o", pch=16 )
    lines( bit_64_idx, apply( data[,bit_64_idx], 2, mean) , col="blue", type = "o", pch=16 )

    legend( "topright", legend=c( "8 bit", "64 bit" ), fill=c("red","blue" ), bty = 0, bg="white" )
    dev.off()
}

print_highest_vs_lowest <- function(  timing, thread_count ) {
    timing <- timing[1:48,c(-1,-2)]

    # --------------------------
    # get distribution of values
    length = length( timing[,1] )
    no_simd_idx = seq( 3, length, 3 )
    bit_16_idx = seq( 2, length, 3 )

    reduced_timing <- timing[-bit_16_idx,]
    reduced_timing <- reduced_timing[ which( reduced_timing == "NW" ), ]

    cbind( reduced_timing[,1:3], apply( reduced_timing[,-c(1:3)], 1, mean ) )

    mean_timing = apply( reduced_timing[,-c(1:3)], 1, mean )

    print( "ratios 1 to x threads" )
    cat( " 8 bit:", mean_timing[1] / mean_timing[(thread_count * 2)-1], "\n" )
    cat( "64 bit:", mean_timing[2] / mean_timing[(thread_count * 2)] )
}

plot_distribution( read_results_csv( "30_03_2015_threads" ), T )
plot_distribution( read_results_csv( "31_03_2015_threads" ), F )

print_highest_vs_lowest( read_results_csv( "30_03_2015_threads" ), 8 )
print_highest_vs_lowest( read_results_csv( "30_03_2015_threads" ), 4 )
print_highest_vs_lowest( read_results_csv( "31_03_2015_threads" ), 4 )

# -------------------
# conversion to GCUPS
residues_db <- 195174196
residues_query <- 513

cells <- residues_db * residues_query

giga_cells <- cells / (1000 * 1000 * 1000)

# for SW 16 bit on 4 threads
mean_runtime_P18080 <- apply( timing[20,-(1:3)], 1, mean )

giga_cells / mean_runtime_P18080

# for SW 8 bit on 4 threads
mean_runtime_O74807 <- mean( c( 0.549311,0.548486,0.548837,0.549139,0.550632,0.549284,0.549413,0.548196,0.551685,0.553249 ) )

giga_cells / mean_runtime_O74807
