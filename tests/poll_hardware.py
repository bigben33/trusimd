import trusimd

hardwares = trusimd.poll_hardware()
for i in range(len(hardwares)):
    print('[{}] {}'.format(i, hardwares[i]))
