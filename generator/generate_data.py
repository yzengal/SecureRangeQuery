import random  
import sys

def generate_2d_positions(n, x_range=(-100, 100), y_range=(-100, 100)):  
    """  
    生成n个二维空间位置  
      
    参数:  
        n (int): 要生成的二维位置的数量  
        x_range (tuple): x坐标的范围，默认为(-100, 100)  
        y_range (tuple): y坐标的范围，默认为(-100, 100)  
          
    返回:  
        list: 包含n个(x, y)坐标的列表，每个坐标都是一个元组  
    """  
    positions = []  
    for _ in range(n):  
        x = int(random.uniform(x_range[0], x_range[1]))  
        y = int(random.uniform(y_range[0], y_range[1])) 
        positions.append((x, y))  
    return positions  

def dumpToFile(fileName, positions):
    with open(fileName, "w") as fout:
        fout.write(str(len(positions)) + "\n")
        for i in range(len(positions)):
            line = "%s %s %s\n" % (i+1, positions[i][0], positions[i][1])
            fout.write(line)
  


if __name__ == "__main__":
     # 检查命令行参数数量  
    if len(sys.argv) != 3:  
        print("Usage: python script.py <n> <filename>")  
        sys.exit(1)  

    # 尝试将第二个命令行参数（索引为1，因为索引从0开始）转换为整数  
    try:  
        n = int(sys.argv[1])  
    except ValueError:  
        print("Error: <n> must be an integer.")  
        sys.exit(1)  

    try:  
        fileName = sys.argv[2]
    except ValueError:  
        print("Error: <filename> must be a valid path.")  
        sys.exit(1)  

    positions = generate_2d_positions(n)  
    dumpToFile(fileName, positions)
    print("[FINISH] Generate Data")