
debugTimes : dict[str,int] ={
    # ~ "writeCommandToNVMController":0,
    # ~ "storeToFlash":0,
    # ~ "clearPageBuffer":0,
    # ~ "beginFlashPage":0,
    # ~ "endFlashPage":0,
    # ~ "STS CCP":0,
    # ~ "STS CONTROLA": 0,
    # ~ "checkNVMError": 0,
    # ~ "STS": 0,
    # ~ "LDS": 0,
}
debugCounts: dict[str,int] ={
}

for k in debugTimes:
    debugCounts[k]=0


def report() -> None:
    for k in debugTimes:
        c=debugCounts[k]
        t=debugTimes[k]/1_000_000
        print(k,f": {c} calls; {t:.2f} msec; {t/c:.3f} ms/call")

def count(name:str, t: int) -> None:
    if name not in debugTimes:
        debugTimes[name]=0
        debugCounts[name]=0
    debugTimes[name]+=t
    debugCounts[name]+=1
