import random
import sys
  
def generate_circles(n, x_range=(-100, 100), y_range=(-100, 100), rad_range=(10, 50)):  
    """  
    生成n个圆的参数（圆心坐标和半径）。  
      
    参数:  
        n (int): 要生成的圆的数量  
        x_range (tuple): x坐标的范围，默认为(-100, 100)  
        y_range (tuple): y坐标的范围，默认为(-100, 100)  
        min_radius (float): 半径的最小值，默认为1  
        max_radius (float): 半径的最大值，默认为50  
          
    返回:  
        list: 包含n个圆的参数（每个圆都是一个(center, radius)的元组，其中center是一个(x, y)的元组）  
    """  
    min_radius, max_radius = rad_range
    circles = []  
    for _ in range(n):  
        x = int(random.uniform(x_range[0], x_range[1]))  
        y = int(random.uniform(y_range[0], y_range[1]))  
        radius = int(random.uniform(min_radius, max_radius))  
        center = (x, y)  
        circles.append((center, radius))  
    return circles  
  

def dumpToFile(fileName, circles):
    with open(fileName, "w") as fout:
        fout.write(str(len(circles)) + "\n")
        for i in range(len(circles)):
            line = "RangeQuery %s %s %s\n" % (circles[i][0][0], circles[i][0][1], circles[i][1])
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

    circles = generate_circles(n)  
    dumpToFile(fileName, circles)
    print("[FINISH] Generate Query")