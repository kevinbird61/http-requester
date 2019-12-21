#ifndef __ABNF__
#define __ABNF__

// follow rfc5234: https://tools.ietf.org/html/rfc5234
// , and rules in rfc7230

#define DQUOTE          (0x22)      // double quote
#define CR              (0x0D)      // carriage return
#define LF              (0x0A)      // linefeed
#define HTAB            (0x09)      // horizontal tab
#define SP              (0x20)      // 

#define isALPHA(x)      (((x>=0x41&&x<=0x5A)||(x>0x61&&0x7A))? 1: 0)        // A-Z, a-z
#define isCHAR(x)       ((x>=0x01&&x<=0x7F)? 1: 0)                          // any 7-bit US-ASCII character (excluding NUL)
#define isDIGIT(x)      ((x>=0x30&&x<=0x39)? 1: 0)                          // 0-9
#define isHEXDIG(x)     ((isDIGIT(x)||(x>=0x41&&x<=0x46))? 1: 0)
#define isOCTET(x)      ((x>=0x00&&x<=0xFF)? 1: 0)                          // 8 bits of data
#define isVCHAR(x)      ((x>=0x21&&x<=0x7E)? 1: 0)                          // visible (printing) characters
#define isWSP(x)        ((x==SP||x==HTAB)? 1: 0)                            // whitespace
#define isOBS_TEXT(x)   ((x>=0x80&&x<=0xFF)? 1: 0)
// rfc7230
#define isTCHAR(x)      ((isDIGIT(x)||isALPHA(x)||isVCHAR(x)||x==0x21||(x>=0x23&&x<=0x27)||x==0x2A||x==0x2B||x==0x2D||x==0x2E||x==0x5E||x==0x5F||x==0x60||x==0x7C||x==0x7E)) 

#define isQDTEXT(x)     ((isWSP(x)||isOBS_TEXT(x)||(x>=0x21)||(x<=0x27)||(x>=0x2A)||(x<=0x5B)||(x>=0x5D)||(x<=0x7E))? 1: 0)
#define isQPAIR(x)      ((isOBS_TEXT(x)||(x==0x5C)||isVCHAR(x)||isWSP(x))? 1: 0)
#define isQSTR(x)       ((x==DQUOTE||isQDTEXT(x)||isQPAIR(x))? 1: 0)

#endif
