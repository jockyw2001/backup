import os

def currentDirFile(dir):
    filenames = os.listdir(dir)
    for fn in filenames:
        if not fn.startswith('.'):
            fullfilename = os.path.join(dir,fn)
            if os.path.isdir(fullfilename):
                print(fullfilename)
                currentDirFile(fullfilename)
currentDirFile("/home/wenshuai.xi/tmp")
