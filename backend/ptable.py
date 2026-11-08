def numOnes(x):
    c=0
    for i in range(8):
        if x & (1<<i):
            c+=1
    return c

def main():
    for i in range(16):
        if numOnes(i) % 2:
            print(1)
        else:
            print(0)


main()
