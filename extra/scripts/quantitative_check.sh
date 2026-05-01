#!/bin/bash

n=$1  # Number of repetitions passed as the first argument
# optional second arg: numeric tolerance (absolute) for comparisons
# usage: ./script/quantitative_check.sh <N> [tolerance]
tolerance=${2:-0.001}
# Use a relaxed epsilon for detecting actual numerical noise vs real differences
# Values smaller than this are treated as zero (floating point precision)
epsilon=1e-3

echo "Tolerance=$tolerance epsilon=$epsilon"

for ((i=1; i<=n; i++)); do
    #echo "Iteration $i"
    printf "."
    ./build/bin/ggg_stochastic_discounted_generate -vx 20 --min-player-outgoing 2 --max-player-outgoing 2 > /dev/null
    val1=$(./build/bin/ggg_stochastic_discounted_solver_strategy ./generated/stochastic_discounted_game_0.dot | awk '/^[[:space:]]*Values:/ { sub(/^[[:space:]]*Values:[[:space:]]*/, ""); gsub(/[\{\},]/, " "); n=split($0, a, /[[:space:]]+/); for (i=2; i<=n; i+=2) if (a[i] != "") printf "%s ", a[i]; print ""; exit }')
    val2=$(./build/bin/ggg_stochastic_discounted_solver_value ./generated/stochastic_discounted_game_0.dot | awk '/^[[:space:]]*Values:/ { sub(/^[[:space:]]*Values:[[:space:]]*/, ""); gsub(/[\{\},]/, " "); n=split($0, a, /[[:space:]]+/); for (i=2; i<=n; i+=2) if (a[i] != "") printf "%s ", a[i]; print ""; exit }')
    val3=$(./build/bin/ggg_stochastic_discounted_solver_objective ./generated/stochastic_discounted_game_0.dot | awk '/^[[:space:]]*Values:/ { sub(/^[[:space:]]*Values:[[:space:]]*/, ""); gsub(/[\{\},]/, " "); n=split($0, a, /[[:space:]]+/); for (i=2; i<=n; i+=2) if (a[i] != "") printf "%s ", a[i]; print ""; exit }')

    #echo "Values: $val1, $val2, $val3"

    #conversion into numbers for tollerance
    read -a arr1 <<< "$val1"
    read -a arr2 <<< "$val2"
    read -a arr3 <<< "$val3"

    len=${#arr1[@]}
    mismatch=0

    for ((j=0; j<len; j++)); do
        n1=${arr1[j]}
        n2=${arr2[j]}
        n3=${arr3[j]}

        # Calculate absolute differences
        diff12=$(awk -v a="$n1" -v b="$n2" 'BEGIN {d=(a-b); if (d<0) d=-d; print d}')
        diff13=$(awk -v a="$n1" -v b="$n3" 'BEGIN {d=(a-b); if (d<0) d=-d; print d}')

        # Check if differences exceed tolerance using relative+absolute comparison
        mismatch_12=$(awk -v a="$n1" -v b="$n2" -v t="$tolerance" 'BEGIN{d=(a-b); if(d<0)d=-d; print(d>t?1:0)}')
        mismatch_13=$(awk -v a="$n1" -v b="$n3" -v t="$tolerance" 'BEGIN{d=(a-b); if(d<0)d=-d; print(d>t?1:0)}')

        if (( mismatch_12 )) || (( mismatch_13 )); then
            echo "Mismatch at position $j: $n1 vs $n2 vs $n3"
            echo "Values: $val1, $val2, $val3"
            mismatch=1
            break
        fi
    done

    if (( mismatch )); then
        echo "Values differ beyond tolerance. Stopping."
        mkdir -p tmp
        cp game1 tmp/failing_game_${i}.ssgame
        echo "Saved failing instance to tmp/failing_game_${i}.ssgame"
        exit 1
    fi
done

echo "$n games matched"
