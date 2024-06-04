import random
import sys
import os

def get_truths(fileName):  
    truths = dict()
    with open(fileName, "r") as fin:
        i, n = 0, 0
        for line in fin:
            line = line.strip()
            if i==0:
                n = int(line)
            elif i<=n*2:
                if i%2==1:
                    qid, sz = map(int, line.split()[:2])
                else:
                    tmpList = set(map(int, line.split()))
                    truths[qid] = tmpList
            i += 1
    return truths

def get_results(fileName):  
    results = dict()
    with open(fileName, "r") as fin:
        i, n = 0, 0
        for line in fin:
            line = line.strip()
            if i==0:
                n = int(line)
            elif i<=n*2:
                if i%2==1:
                    qid, sz, cand = map(int, line.split()[:3])
                else:
                    tmpList = set(map(int, line.split()))
                    results[qid] = [tmpList, cand]
            elif i==n*2+1:
                if line.find("Query Log") >= 0:
                    hasLog = True
                else:
                    hasLog = False
            elif i==n*2+2:
                if hasLog:
                    results['log'] = line
            i += 1
    return results

def dumpToFile(fileName, truths, results):
    total_recall, total_precision = 0.0, 0.0
    lines = []
    for qid in sorted(truths.keys()):
        if qid == 'log':
            continue
        true_answers = truths[qid]
        recv_answers, recv_cand = results[qid]
        tp = len(true_answers.intersection(recv_answers))
        assert tp==len(recv_answers), "recv_answer has false positive"
        fn = len(true_answers - recv_answers)
        fp = recv_cand - tp
        cur_recall = tp / (tp + fn) if (tp + fn) > 0 else 0.0
        cur_precision = tp / (tp + fp) if (tp + fp) > 0 else 0.0 
        line = f"recall = {cur_recall:.4f}, cur_precision = {cur_precision:.4f}"
        print(line)
        lines.append(line)
        total_recall += cur_recall
        total_precision += cur_precision

    query_num = len(truths)
    if query_num == 0:
        avg_recall, avg_precision = 0.0, 0.0
    else:
        avg_recall = total_recall / query_num
        avg_precision = total_precision / query_num
    
    avg_line = "[AVERAGE] recall = %.6f, precision = %.6f\n" % (avg_recall, avg_precision)
    if not fileName:                # fileName is empty
        print(avg_line)
    else:
        with open(fileName, "w") as fout:
            for line in lines:
                fout.write(line + "\n")
            fout.write(avg_line)
            if 'log' in results:
                long_line = results['log']
                fout.write(long_line)


if __name__ == "__main__":
     # 检查命令行参数数量  
    if len(sys.argv) < 3:  
        print("Usage: python compute_query_accuracy.py <truthfile> <resultfile> <outputfile(optional)>")  
        sys.exit(1)  

    # 尝试将第二个命令行参数（索引为1，因为索引从0开始）转换为整数  
    try:  
        truth_fileName = sys.argv[1]  
    except ValueError:  
        print("Error: <filename> must be a valid path.")  
        sys.exit(1)  

    try:  
        result_fileName = sys.argv[2]
    except ValueError:  
        print("Error: <filename> must be a valid path.")  
        sys.exit(1)  
    
    if len(sys.argv) > 3:
        try:  
            output_fileName = sys.argv[3]
        except ValueError:  
            print("Error: <filename> must be a valid path.")  
            sys.exit(1) 
    else:
        output_fileName = ""

    truths = get_truths(truth_fileName)  
    results = get_results(result_fileName)
    dumpToFile(output_fileName, truths, results)

    print("[FINISH] Compute Query Accuracy")