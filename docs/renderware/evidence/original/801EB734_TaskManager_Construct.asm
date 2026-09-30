; target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
; TaskManager_Construct: [801EB734, 801EB790), 0x5c bytes
; classification: shared root and ordered-group construction
; RAM address  DOL offset  raw word  instruction
801EB734  001E4FF4  9421FFF0  stwu         r1, -0x10(r1)
801EB738  001E4FF8  7C0802A6  mflr         r0
801EB73C  001E4FFC  90010014  stw          r0, 0x14(r1)
801EB740  001E5000  93E1000C  stw          r31, 0xc(r1)
801EB744  001E5004  7C7F1B78  mr           r31, r3
801EB748  001E5008  38600028  li           r3, 0x28
801EB74C  001E500C  481B5C35  bl           0x803a1380
801EB750  001E5010  7C601B79  or.          r0, r3, r3
801EB754  001E5014  41820010  beq          0x801eb764
801EB758  001E5018  38800000  li           r4, 0
801EB75C  001E501C  4BE638B9  bl           0x8004f014
801EB760  001E5020  7C601B78  mr           r0, r3
801EB764  001E5024  901F0000  stw          r0, 0(r31)
801EB768  001E5028  7FE3FB78  mr           r3, r31
801EB76C  001E502C  4BFFFDD1  bl           0x801eb53c
801EB770  001E5030  7FE3FB78  mr           r3, r31
801EB774  001E5034  4BFFFD31  bl           0x801eb4a4
801EB778  001E5038  80010014  lwz          r0, 0x14(r1)
801EB77C  001E503C  7FE3FB78  mr           r3, r31
801EB780  001E5040  83E1000C  lwz          r31, 0xc(r1)
801EB784  001E5044  7C0803A6  mtlr         r0
801EB788  001E5048  38210010  addi         r1, r1, 0x10
801EB78C  001E504C  4E800020  blr
