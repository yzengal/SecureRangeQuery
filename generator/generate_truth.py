import random
import sys

def is_point_in_circle(point, circle):  
    """  
    判断一个二维点是否在圆内（包括圆上）。  
      
    参数:  
        point (tuple): 二维点，格式为 (x, y)  
        center (tuple): 圆心，格式为 (x, y)  
        radius (float): 圆的半径  
          
    返回:  
        bool: 如果点在圆内（包括圆上），返回 True；否则返回 False  
    """  
    # 计算点到圆心的距离的平方  
    center, radius = circle
    distance_squared = (point[0] - center[0]) ** 2 + (point[1] - center[1]) ** 2  
    # 判断距离的平方是否小于等于半径的平方  
    return distance_squared <= radius ** 2  
  
def get_circles(fileName):  
    circles = []
    with open(fileName, "r") as fin:
        i, n = 0, 0
        for line in fin:
            line = line.strip()
            if i==0:
                n = int(line)
            elif i<=n:
                x, y, radius = map(float, line.split()[1:4])
                center = (x, y)  
                circles.append((center, radius))  
            i += 1
    return circles
  
def get_positions(fileNames):  
    positions = []
    for fileName in fileNames:
        with open(fileName, "r") as fin:
            i, n = 0, 0
            for line in fin:
                line = line.strip()
                if i==0:
                    n = int(line)
                elif i<=n:
                    id, x, y = map(int, line.split()[:3])
                    point = (x, y)  
                    positions.append((id, point))  
                i += 1
    return positions

def dumpToFile(fileName, positions, circles):
    with open(fileName, "w") as fout:
        fout.write("%d\n" % (len(circles)))
        for i,circle in enumerate(circles):
            tmpList = []
            for (pid, point) in positions:
                if is_point_in_circle(point, circle):
                    tmpList.append(pid)
            fout.write("%d %d\n" % (i+1, len(tmpList)))
            line = " ".join(map(str, tmpList)) + "\n"
            fout.write(line)


if __name__ == "__main__":
     # 检查命令行参数数量  
    if len(sys.argv) < 4:  
        print("Usage: python generate_truth.py <datafile_1> ... <datafile_n> <queryfile> <resultfile>")  
        sys.exit(1)  

    # 尝试将第二个命令行参数（索引为1，因为索引从0开始）转换为整数  
    try:  
        data_fileNames = sys.argv[1:-2]  
        print(data_fileNames)
    except ValueError:  
        print("Error: <filename> must be a valid path.")  
        sys.exit(1)  

    try:  
        query_fileName = sys.argv[-2]
    except ValueError:  
        print("Error: <filename> must be a valid path.")  
        sys.exit(1)  

    try:  
        truth_fileName = sys.argv[-1]
    except ValueError:  
        print("Error: <filename> must be a valid path.")  
        sys.exit(1) 

    positions = get_positions(data_fileNames)  
    circles = get_circles(query_fileName)
    dumpToFile(truth_fileName, positions, circles)

    print("[FINISH] Generate Ground Truth")