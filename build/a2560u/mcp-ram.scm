(define memories
  '((memory flash (address (#x20000 . #x47fff)) (type ROM) (section text cfar code))
    (memory dataRAM (address (#x48000 . #x4ffff)) (type RAM) (section stack sstack heap BSS vectors6))
    (memory Vector (address (#x0000 . #x03ff)) (section VECTORS))
    ))
