#!/usr/bin/env python3


import scienceplots
import editdistance

import matplotlib
import matplotlib.pyplot as plt
import numpy as np



strand_file = 'dataset/clustered-nanopore-reads-dataset/Clusters.txt'
center_file = 'dataset/clustered-nanopore-reads-dataset/Centers.txt'

length_strand = 110
ins_sts = [0 for i in range(length_strand)]
del_sts = [0 for i in range(length_strand)]
sub_sts = [0 for i in range(length_strand)]


# Costs for the operations
INS_COST = 1
DEL_COST = 1
SUB_COST = 2

def find_minimum_edit_distance(source_string, target_string) :

    # Create a dp matrix of dimension (source_string + 1) x (destination_matrix + 1)
    dp = [[0] * (len(source_string) + 1) for i in range(len(target_string) + 1)]

    # Initialize the required values of the matrix
    for i in range(1, len(target_string) + 1) :
        dp[i][0] = dp[i - 1][0] + INS_COST
    for i in range(1, len(source_string) + 1) :
        dp[0][i] = dp[0][i - 1] + DEL_COST

    # Maintain the record of opertions done
    # Record is one tuple. Eg : (INSERT, 'a') or (SUBSTITUTE, 'e', 'r') or (DELETE, 'j')
    operations_performed = []
    sts = []

    # Build the matrix following the algorithm
    for i in range(1, len(target_string) + 1) :
        for j in range(1, len(source_string) + 1) :
            if source_string[j - 1] == target_string[i - 1] :
                dp[i][j] = dp[i - 1][j - 1]
            else :
                dp[i][j] =  min(dp[i - 1][j] + INS_COST, \
                                dp[i - 1][j - 1] + SUB_COST, \
                                dp[i][j - 1] + DEL_COST)

    # Initialization for backtracking
    i = len(target_string)
    j = len(source_string)

    # Backtrack to record the operation performed
    while (i != 0 and j != 0) :
        # If the character of the source string is equal to the character of the destination string,
        # no operation is performed
        if target_string[i - 1] == source_string[j - 1] :
            i -= 1
            j -= 1
        else :
            # Check if the current element is derived from the upper-left diagonal element
            if dp[i][j] == dp[i - 1][j - 1] + SUB_COST :
                operations_performed.append(('SUBSTITUTE', source_string[j - 1], target_string[i - 1]))
                sts.append(('s', j - 1))
                i -= 1
                j -= 1
            # Check if the current element is derived from the upper element
            elif dp[i][j] == dp[i - 1][j] + INS_COST :
                operations_performed.append(('INSERT', target_string[i - 1]))
                sts.append(('i', j - 1))
                i -= 1
            # Check if the current element is derived from the left element
            else :
                operations_performed.append(('DELETE', source_string[j - 1]))
                sts.append(('d', j - 1))
                j -= 1

    # If we reach top-most row of the matrix
    while (j != 0) :
        operations_performed.append(('DELETE', source_string[j - 1]))
        sts.append(('d', j - 1))
        j -= 1

    # If we reach left-most column of the matrix
    while (i != 0) :
        operations_performed.append(('INSERT', target_string[i - 1]))
        sts.append(('i', j - 1))
        i -= 1

    # Reverse the list of operations performed as we have operations in reverse
    # order because of backtracking
    operations_performed.reverse()
    return [dp[len(target_string)][len(source_string)], operations_performed, sts]

def edit_distance_sts(sts):

    for operation in sts:
        index = operation[1]
        if (operation[0] == 'i'):
            ins_sts[index]  = ins_sts[index] + 1
        elif (operation[0] == 'd'):
            del_sts[index]  = del_sts[index] + 1
        else:
            sub_sts[index]  = sub_sts[index] + 1

def p_plot(ax,item, item_name):
    ax.plot(item, linewidth=3)
    median = np.median(item)
    print(median)
    base = [median for i in range(len(item))]
    ax.plot(base, linewidth=3, color='red')
    ax.annotate('Pr =' + '{:.4f}'.format(median), xy =(5, median),
                xytext =(15.5, median + 0.1),
                arrowprops = dict(facecolor ='red',
                                  shrink = 0.01),)
    ax.set_xlabel("Probability distribution of \n"+ item_name + " at each base")
    ax.grid()

if __name__ == "__main__":


    f_strand = open(strand_file, 'r')
    f_center = open(center_file, 'r')

    ground_truth = []
    list_of_ed = []
    counter = 0;
    while True:
        line = f_strand.readline();
        if not line:
            break;

        if (line[0] == '='):
            ground_truth = f_center.readline();
            if not ground_truth:
                break
        else:
            distance, operations_performed, sts = find_minimum_edit_distance(ground_truth[0:length_strand - 1], line[0:length_strand - 1])
            edit_distance_sts(sts)
            list_of_ed.append(distance)

        counter = counter + 1
        if (counter >= 20000):
            break

    #plt.style.use(['nature'])
    plt.rcParams.update({'font.size': 22})
    plt.rcParams.update({'font.weight': "bold"})
    plt.rcParams.update({'axes.linewidth': 2})
    plt.rcParams.update({'axes.titleweight': 'bold'})
    plt.rcParams.update({'axes.labelweight': 'bold'})

    plt.rcParams.update({'savefig.dpi': 300})


    #matplotlib.use("pgf")
    #plt.rcParams.update({
    #    'pgf.texsystem': "pdflatex",
    #    'font.family': 'serif',
    #    'text.usetex': True,
    #    'pgf.rcfonts': False,
    #})



    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, sharey=True)




    fig_size = 10
    fig.set_figheight(fig_size)
    fig.set_figwidth(fig_size * 4)

    p_plot(ax1,np.array(ins_sts)/float(counter ), "insertion error")
    p_plot(ax2,np.array(del_sts)/float(counter ), "deletion error")
    p_plot(ax3,np.array(sub_sts)/float(counter ), "substitution error")

    #plt.savefig('histogram.png')

    plt.show()







#pip install latex
#pip install SciencePlots
#sudo apt install texlive texlive-latex-extra texlive-fonts-recommended dvipng cm-super



exit(0)


seq1 = 'ACCATAATGCGTGGGGCCG'
seq2 = 'ACTGCATAATGCGTGGGAC'

source_string = seq1

target_string = seq2
distance, operations_performed, sts = find_minimum_edit_distance(ground_truth, line)


# Count the number of individual operations
insertions, deletions, substitutions = 0, 0, 0
for i in operations_performed :
    if i[0] == 'INSERT' :
        insertions += 1
    elif i[0] == 'DELETE' :
        deletions += 1
    else :
        substitutions += 1

# Print the results
print("Minimum edit distance : {}".format(distance))
print("Number of insertions : {}".format(insertions))
print("Number of deletions : {}".format(deletions))
print("Number of substitutions : {}".format(substitutions))
print("Total number of operations : {}".format(insertions + deletions + substitutions))

print("Actual Operations :")
for i in range(len(operations_performed)) :

    if operations_performed[i][0] == 'INSERT' :
        print("{}) {} : {}".format(i + 1, operations_performed[i][0], operations_performed[i][1]))
    elif operations_performed[i][0] == 'DELETE' :
        print("{}) {} : {}".format(i + 1, operations_performed[i][0], operations_performed[i][1]))
    else :
        print("{}) {} : {} by {}".format(i + 1, operations_performed[i][0], operations_performed[i][1], operations_performed[i][2]))
