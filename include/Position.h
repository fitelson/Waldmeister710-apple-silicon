/* *************************************************************************
 * Position.h
 *
 * ADT für erweiterte Positionen
 * 
 *
 * Jean-Marie Gaillourdet 13.5.2003
 *
 * ************************************************************************* */

#ifndef POSITION_H
#define POSITION_H

typedef unsigned long Position;

/* Position ist ein Bitfeld mit folgender Belegung:
 * 31.    bit : into_i    wird in i überlappt?
 * 30.    bit : i_right   wird von i die rechte Seite verwendet?
 * 29.    bit : j_right   wird von j die rechte Seite verwendet?
 * 0.-28. bit : Position der Überlappung in der Termdarstellung
 */
#define POS_MASK     0x1FFFFFFF
#define INTO_I_MASK  0x80000000
#define I_RIGHT_MASK 0x40000000
#define J_RIGHT_MASK 0x20000000
#define XBITS_MASK   (INTO_I_MASK|I_RIGHT_MASK|J_RIGHT_MASK)
#define LOWER_3_MASK 0x00000007
#define LOWEST_MASK  0x00000001

#define POS_into_i(p)  (((BOOLEAN) (INTO_I_MASK  & (p))) ? TRUE : FALSE)
#define POS_i_right(p) (((BOOLEAN) (I_RIGHT_MASK & (p))) ? TRUE : FALSE)
#define POS_j_right(p) (((BOOLEAN) (J_RIGHT_MASK & (p))) ? TRUE : FALSE)
#define POS_pos(p)     (POS_MASK & (p))
#define POS_xbits(p)   ((( XBITS_MASK & (p)) >> 29) & LOWER_3_MASK)

#define POS_set_into_i(p,v)  ((p) = ((((LOWEST_MASK & (v)) << 31) & INTO_I_MASK)  | ((~INTO_I_MASK) & (p))))
#define POS_set_i_right(p,v) ((p) = ((((LOWEST_MASK & (v)) << 30) & I_RIGHT_MASK) | ((~I_RIGHT_MASK)& (p))))
#define POS_set_j_right(p,v) ((p) = ((((LOWEST_MASK & (v)) << 29) & J_RIGHT_MASK) | ((~J_RIGHT_MASK)& (p))))
#define POS_set_pos(p,v)     ((p) = ((  POS_MASK    & (v))                        | ((~POS_MASK)    & (p))))
#define POS_set_xbits(p,v)   ((p) = ((((LOWER_3_MASK& (v)) << 29) & XBITS_MASK)   | ((~XBITS_MASK)  & (p))))

#define /* Position */ POS_mk_pos(/* unsigned */ p, /* BOOLEAN */ into_i, /* BOOLEAN */ i_right, /* BOOLEAN */ j_right) \
  (((p) & POS_MASK) | ((into_i)?INTO_I_MASK:0) |((i_right)?I_RIGHT_MASK:0) | ((j_right)?J_RIGHT_MASK:0))

#define XBIT_into_i(xbits)  (((xbits) >> 2) & 1)
#define XBIT_i_right(xbits) (((xbits) >> 1) & 1)
#define XBIT_j_right(xbits) (((xbits) >> 0) & 1)

#define maximalPosition() ULONG_MAX

#define DruckePos(xp) (IO_DruckeFlex("<%u,%u,%u,%u>",POS_into_i(xp),POS_i_right(xp),POS_j_right(xp),POS_pos(xp)))

#endif /* POSITION_H */
