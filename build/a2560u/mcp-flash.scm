(define memories
  '((memory flash (address (#xe00000 . #xe7ffff)) (fill 0) (type ROM)
       (section (reset #xe00000) (VECTORS #xe00008) text cfar code))
    (memory dataRAM (address (#x00400 . #xffff)) (type RAM) (section stack sstack heap BSS vectors6))
    ))
