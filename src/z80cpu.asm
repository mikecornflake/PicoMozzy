
;
;
;      \\\              ///
;     /~~~~~~~~~\      ///
;     \______   /     ///
;        \\\/  ####  /####
;         \/  #    # #//  #
;         /  /#    #/#/   #   VZ80 - Virtual Z80 for PC
;        /  /\ #### /#    #   32-bit z80 emulator
;       /  /\\#    #/#    #   Coding: Alistair Shilton
;      /  /  \#\  /#/#    #           apsh@ee.unimelb.edu.au
;     /   ~~~~~####/  ####            http://
;     \_________////
;              \\//
;               \/
;
;

;
; Compilation: nasm -fcoff z80.asm
;

                BITS 32
                ALIGN   4,db 0

;
; Question: Interupt mode 0.  Multibyte ops (eg DDxx, CALL nn).
;           Z80 source docs - only mention the RST ops.  No memtion made
;               of the result of putting multibyte ops on the bus.
;           LaLond - The 8080, 8085, and z80 - Hardware, Software,
;                    Programming, Interfacing and Troubleshooting.
;               pg 403: "If a CALL instruction is placed on the data
;                       bus in responce to an INTA* signal, the Z80
;                       will perform two memory reads after receiving
;                       the CALL.  This means that the 8259A is not
;                       useable as a priority interupt controller in a
;                       Z80 system".
;           Zaks - Programming the Z80.
;               pg 501:
;                                Mode 0
;                                  ||
;                                  \/
;                        +---------------------+
;                        |  Disable Interupts  |
;                        |   IFF1 = IFF2 = 0   |
;                        +---------------------+
;                                  ||
;                                  \/
;                        +---------------------+
;                        |   Read first byte   |
;                        |   of instruction    |
;                        |    (M1, IRQ low)    |
;                        +---------------------+
;                                  ||
;             ++------------------>||
;             ||                   \/
;             ||                   /\
;             ||         +--------+  +---------+
;             ||         |     More bytes      |
;             ||        <     required for      > ---> (execute operation)
;             ||         |    instruction?     |
;             ||         +--------+  +---------+
;             ||                   \/
;             ||                   ||
;             ||                   \/
;             ||         +---------------------+
;             ||         |  Read next byte     |
;             ||         | (Normal mem. read   |
;             ||         | with PC stationary) |
;             ||         +---------------------+
;             ||                   ||
;             ++-------------------++
;
;              but, once again, only CALL and RST ops are mentioned.
;
;           Most other sources - multibyte ops all come from the interupt
;               source (usually vaguely worded, though).
;
; But no source I can find explicitly deals with DD/FD/ED/CB prefixed ops.
; These have multiple *opfetches*, not just one opfetch followed by some
; memory read cycles from PC.  I have dealt with this case as follows:
;
;   Any instruction will be read to completion.  That includes very extended
;   undocced DDFDDDCB... ops.  I also assume that all opfetches in this
;   sequence (but not data afterwards) will be special and have the usual
;   extra wait states and also M1, IRQ etc low to indicate this (and no PC
;   incrementing).  This uses the _z80_ack_INT function.  Subsequent PC data
;   reads (eg. the nn in CALL nn) will be done as described in Zaks (that
;   is, normal reads from PC, but without the usual PC incrementation).
;
; NOTE: the BIT flag is reliant on a hidden register in the z80.  This is
;       explained in Sean Young's docs, but details are largely unknown.
;       Hence, the only time the hidden register is modified in when offset
;       memory reads from (HL+d) are made (although the effect of the
;       following is also known: ADD HL,xx , JR d , LD r,r').  The undoced
;       register copies an address (word) HL+d prior to the read, of which
;       the upper byte is what is needed by BIT (HL+d).  The hidden register
;       is called hidden_reg here.  Note that the BIT (HL) instructions
;       set flag bits 3/5 using the hidden register also, so if more info
;       comes to hand it should not be difficult to implement the the hidden
;       register correctly.



; The caller must provide the following functions:
;
; data bus key: w = wait bus
;               r = refresh bus
;               d = data bus
;               a = address bus
;               c = clock bus
;
; output: data provided to external function
; input: extern function provides this data


                EXTERN _z80_sig_error  ; Signal emulation error.
                EXTERN _z80_ack_reset  ; Ack reset
                EXTERN _z80_ack_busrq  ; Ack busrq, pass control (input w)
                EXTERN _z80_ack_halt   ; Ack halt
                EXTERN _z80_ack_NMI    ; Ack NMI (output ra)
                EXTERN _z80_ack_INT    ; Ack INT (output a, input wd)
                EXTERN _z80_opfetch    ; Opfetch cycle (output ra, input wd)
                EXTERN _z80_wr_mem     ; Write to memory (output ad, in w)
                EXTERN _z80_rd_mem     ; Read from memory (output a, in wd)
                EXTERN _z80_wr_io      ; Write to io (output ad, in w)
                EXTERN _z80_rd_io      ; Read from io (output a, in wd)


; These functions are provided by the emulator

                GLOBAL _z80_wait_bus
                GLOBAL _z80_rfsh_bus
                GLOBAL _z80_data_bus
                GLOBAL _z80_addr_bus
                GLOBAL _z80_clk_bus

                GLOBAL _z80_tab_num_bus
                GLOBAL _z80_tab_addr_bus
                GLOBAL _z80_tab_wr_wait_bus
                GLOBAL _z80_tab_rd_wait_bus

                GLOBAL _z80_reti_count_bus

                GLOBAL _z80_init
                GLOBAL _z80_cycle

                GLOBAL _z80_set_reset
                GLOBAL _z80_set_busrq
                GLOBAL _z80_set_NMI
                GLOBAL _z80_set_INT
                GLOBAL _z80_res_INT
                GLOBAL _z80_set_wait

                GLOBAL _z80_set_mem_write_none
                GLOBAL _z80_set_mem_write_direct
                GLOBAL _z80_set_mem_write_indirect

                GLOBAL _z80_set_mem_read_none
                GLOBAL _z80_set_mem_read_direct
                GLOBAL _z80_set_mem_read_indirect

                GLOBAL _z80_set_mem_opread_none
                GLOBAL _z80_set_mem_opread_direct
                GLOBAL _z80_set_mem_opread_indirect

                GLOBAL _z80_set_mem_write_none_naw
                GLOBAL _z80_set_mem_write_direct_naw
                GLOBAL _z80_set_mem_write_indirect_naw

                GLOBAL _z80_set_mem_read_none_naw
                GLOBAL _z80_set_mem_read_direct_naw
                GLOBAL _z80_set_mem_read_indirect_naw

                GLOBAL _z80_set_mem_opread_none_naw
                GLOBAL _z80_set_mem_opread_direct_naw
                GLOBAL _z80_set_mem_opread_indirect_naw







                SECTION .data

; Jump table for main opcode set

                ALIGN   4,db 0

main_opt:
main_0x:        dd Z_NOP       , Z_LD_BCcnn  , Z_LD_bBCbcA , Z_INC_BC    , Z_INC_B     , Z_DEC_B     , Z_LD_Bcn    , Z_RLCA      , Z_EX_AFcAFp , Z_ADD_HLcBC , Z_LD_AcbBCb , Z_DEC_BC    , Z_INC_C     , Z_DEC_C     , Z_LD_Ccn    , Z_RRCA
main_1x:        dd Z_DJNZ_e    , Z_LD_DEcnn  , Z_LD_bDEbcA , Z_INC_DE    , Z_INC_D     , Z_DEC_D     , Z_LD_Dcn    , Z_RLA       , Z_JR_e      , Z_ADD_HLcDE , Z_LD_AcbDEb , Z_DEC_DE    , Z_INC_E     , Z_DEC_E     , Z_LD_Ecn    , Z_RRA       
main_2x:        dd Z_JR_NZce   , Z_LD_HLcnn  , Z_LD_bnnbcHL, Z_INC_HL    , Z_INC_H     , Z_DEC_H     , Z_LD_Hcn    , Z_DAA       , Z_JR_Zce    , Z_ADD_HLcHL , Z_LD_HLcbnnb, Z_DEC_HL    , Z_INC_L     , Z_DEC_L     , Z_LD_Lcn    , Z_CPL       
main_3x:        dd Z_JR_NCce   , Z_LD_SPcnn  , Z_LD_bnnbcA , Z_INC_SP    , Z_INC_bHLb  , Z_DEC_bHLb  , Z_LD_bHLbcn , Z_SCF       , Z_JR_Cce    , Z_ADD_HLcSP , Z_LD_Acbnnb , Z_DEC_SP    , Z_INC_A     , Z_DEC_A     , Z_LD_Acn    , Z_CCF       
main_4x:        dd Z_LD_BcB    , Z_LD_BcC    , Z_LD_BcD    , Z_LD_BcE    , Z_LD_BcH    , Z_LD_BcL    , Z_LD_BcbHLb , Z_LD_BcA    , Z_LD_CcB    , Z_LD_CcC    , Z_LD_CcD    , Z_LD_CcE    , Z_LD_CcH    , Z_LD_CcL    , Z_LD_CcbHLb , Z_LD_CcA    
main_5x:        dd Z_LD_DcB    , Z_LD_DcC    , Z_LD_DcD    , Z_LD_DcE    , Z_LD_DcH    , Z_LD_DcL    , Z_LD_DcbHLb , Z_LD_DcA    , Z_LD_EcB    , Z_LD_EcC    , Z_LD_EcD    , Z_LD_EcE    , Z_LD_EcH    , Z_LD_EcL    , Z_LD_EcbHLb , Z_LD_EcA    
main_6x:        dd Z_LD_HcB    , Z_LD_HcC    , Z_LD_HcD    , Z_LD_HcE    , Z_LD_HcH    , Z_LD_HcL    , Z_LD_HcbHLb , Z_LD_HcA    , Z_LD_LcB    , Z_LD_LcC    , Z_LD_LcD    , Z_LD_LcE    , Z_LD_LcH    , Z_LD_LcL    , Z_LD_LcbHLb , Z_LD_LcA    
main_7x:        dd Z_LD_bHLbcB , Z_LD_bHLbcC , Z_LD_bHLbcD , Z_LD_bHLbcE , Z_LD_bHLbcH , Z_LD_bHLbcL , Z_HALT      , Z_LD_bHLbcA , Z_LD_AcB    , Z_LD_AcC    , Z_LD_AcD    , Z_LD_AcE    , Z_LD_AcH    , Z_LD_AcL    , Z_LD_AcbHLb , Z_LD_AcA    
main_8x:        dd Z_ADD_AcB   , Z_ADD_AcC   , Z_ADD_AcD   , Z_ADD_AcE   , Z_ADD_AcH   , Z_ADD_AcL   , Z_ADD_AcbHLb, Z_ADD_AcA   , Z_ADC_AcB   , Z_ADC_AcC   , Z_ADC_AcD   , Z_ADC_AcE   , Z_ADC_AcH   , Z_ADC_AcL   , Z_ADC_AcbHLb, Z_ADC_AcA   
main_9x:        dd Z_SUB_AcB   , Z_SUB_AcC   , Z_SUB_AcD   , Z_SUB_AcE   , Z_SUB_AcH   , Z_SUB_AcL   , Z_SUB_AcbHLb, Z_SUB_AcA   , Z_SBC_AcB   , Z_SBC_AcC   , Z_SBC_AcD   , Z_SBC_AcE   , Z_SBC_AcH   , Z_SBC_AcL   , Z_SBC_AcbHLb, Z_SBC_AcA   
main_Ax:        dd Z_AND_AcB   , Z_AND_AcC   , Z_AND_AcD   , Z_AND_AcE   , Z_AND_AcH   , Z_AND_AcL   , Z_AND_AcbHLb, Z_AND_AcA   , Z_XOR_AcB   , Z_XOR_AcC   , Z_XOR_AcD   , Z_XOR_AcE   , Z_XOR_AcH   , Z_XOR_AcL   , Z_XOR_AcbHLb, Z_XOR_AcA   
main_Bx:        dd Z_OR_AcB    , Z_OR_AcC    , Z_OR_AcD    , Z_OR_AcE    , Z_OR_AcH    , Z_OR_AcL    , Z_OR_AcbHLb , Z_OR_AcA    , Z_CP_AcB    , Z_CP_AcC    , Z_CP_AcD    , Z_CP_AcE    , Z_CP_AcH    , Z_CP_AcL    , Z_CP_AcbHLb , Z_CP_AcA    
main_Cx:        dd Z_RET_NZ    , Z_POP_BC    , Z_JP_NZcnn  , Z_JP_nn     , Z_CALL_NZcnn, Z_PUSH_BC   , Z_ADD_Acn   , Z_RST_00H   , Z_RET_Z     , Z_RET       , Z_JP_Zcnn   , Z_CB_PREF   , Z_CALL_Zcnn , Z_CALL_nn   , Z_ADC_Acn   , Z_RST_08H   
main_Dx:        dd Z_RET_NC    , Z_POP_DE    , Z_JP_NCcnn  , Z_OUT_bAnbcA, Z_CALL_NCcnn, Z_PUSH_DE   , Z_SUB_Acn   , Z_RST_10H   , Z_RET_C     , Z_EXX       , Z_JP_Ccnn   , Z_IN_AcbAnb , Z_CALL_Ccnn , Z_DD_PREF   , Z_SBC_Acn   , Z_RST_18H   
main_Ex:        dd Z_RET_PO    , Z_POP_HL    , Z_JP_POcnn  , Z_EX_bSPbcHL, Z_CALL_POcnn, Z_PUSH_HL   , Z_AND_Acn   , Z_RST_20H   , Z_RET_PE    , Z_JP_HL     , Z_JP_PEcnn  , Z_EX_DEcHL  , Z_CALL_PEcnn, Z_ED_PREF   , Z_XOR_Acn   , Z_RST_28H   
main_Fx:        dd Z_RET_P     , Z_POP_AF    , Z_JP_Pcnn   , Z_DI        , Z_CALL_Pcnn , Z_PUSH_AF   , Z_OR_Acn    , Z_RST_30H   , Z_RET_M     , Z_LD_SPcHL  , Z_JP_Mcnn   , Z_EI        , Z_CALL_Mcnn , Z_FD_PREF   , Z_CP_Acn    , Z_RST_38H

main_opt_DD:
main_0x_DD:     dd Z_NOP         , Z_LD_BCcnn    , Z_LD_bBCbcA   , Z_INC_BC      , Z_INC_B       , Z_DEC_B       , Z_LD_Bcn      , Z_RLCA        , Z_EX_AFcAFp   , Z_ADD_HLcBC   , Z_LD_AcbBCb   , Z_DEC_BC      , Z_INC_C       , Z_DEC_C       , Z_LD_Ccn      , Z_RRCA      
main_1x_DD:     dd Z_DJNZ_e      , Z_LD_DEcnn    , Z_LD_bDEbcA   , Z_INC_DE      , Z_INC_D       , Z_DEC_D       , Z_LD_Dcn      , Z_RLA         , Z_JR_e        , Z_ADD_HLcDE   , Z_LD_AcbDEb   , Z_DEC_DE      , Z_INC_E       , Z_DEC_E       , Z_LD_Ecn      , Z_RRA       
main_2x_DD:     dd Z_JR_NZce     , Z_LD_HLcnn    , Z_LD_bnnbcHL  , Z_INC_HL      , Z_INC_H       , Z_DEC_H       , Z_LD_Hcn      , Z_DAA         , Z_JR_Zce      , Z_ADD_HLcHL   , Z_LD_HLcbnnb  , Z_DEC_HL      , Z_INC_L       , Z_DEC_L       , Z_LD_Lcn      , Z_CPL       
main_3x_DD:     dd Z_JR_NCce     , Z_LD_SPcnn    , Z_LD_bnnbcA   , Z_INC_SP      , ZxD_INC_bHLb  , ZxD_DEC_bHLb  , ZxD_LD_bHLbcn , Z_SCF         , Z_JR_Cce      , Z_ADD_HLcSP   , Z_LD_Acbnnb   , Z_DEC_SP      , Z_INC_A       , Z_DEC_A       , Z_LD_Acn      , Z_CCF       
main_4x_DD:     dd Z_LD_BcB      , Z_LD_BcC      , Z_LD_BcD      , Z_LD_BcE      , Z_LD_BcH      , Z_LD_BcL      , ZxD_LD_BcbHLb , Z_LD_BcA      , Z_LD_CcB      , Z_LD_CcC      , Z_LD_CcD      , Z_LD_CcE      , Z_LD_CcH      , Z_LD_CcL      , ZxD_LD_CcbHLb , Z_LD_CcA    
main_5x_DD:     dd Z_LD_DcB      , Z_LD_DcC      , Z_LD_DcD      , Z_LD_DcE      , Z_LD_DcH      , Z_LD_DcL      , ZxD_LD_DcbHLb , Z_LD_DcA      , Z_LD_EcB      , Z_LD_EcC      , Z_LD_EcD      , Z_LD_EcE      , Z_LD_EcH      , Z_LD_EcL      , ZxD_LD_EcbHLb , Z_LD_EcA    
main_6x_DD:     dd Z_LD_HcB      , Z_LD_HcC      , Z_LD_HcD      , Z_LD_HcE      , Z_LD_HcH      , Z_LD_HcL      , ZDD_LD_HcbHLb , Z_LD_HcA      , Z_LD_LcB      , Z_LD_LcC      , Z_LD_LcD      , Z_LD_LcE      , Z_LD_LcH      , Z_LD_LcL      , ZDD_LD_LcbHLb , Z_LD_LcA    
main_7x_DD:     dd ZxD_LD_bHLbcB , ZxD_LD_bHLbcC , ZxD_LD_bHLbcD , ZxD_LD_bHLbcE , ZDD_LD_bHLbcH , ZDD_LD_bHLbcL , ZDD_HALT      , ZxD_LD_bHLbcA , Z_LD_AcB      , Z_LD_AcC      , Z_LD_AcD      , Z_LD_AcE      , Z_LD_AcH      , Z_LD_AcL      , ZxD_LD_AcbHLb , Z_LD_AcA    
main_8x_DD:     dd Z_ADD_AcB     , Z_ADD_AcC     , Z_ADD_AcD     , Z_ADD_AcE     , Z_ADD_AcH     , Z_ADD_AcL     , ZxD_ADD_AcbHLb, Z_ADD_AcA     , Z_ADC_AcB     , Z_ADC_AcC     , Z_ADC_AcD     , Z_ADC_AcE     , Z_ADC_AcH     , Z_ADC_AcL     , ZxD_ADC_AcbHLb, Z_ADC_AcA   
main_9x_DD:     dd Z_SUB_AcB     , Z_SUB_AcC     , Z_SUB_AcD     , Z_SUB_AcE     , Z_SUB_AcH     , Z_SUB_AcL     , ZxD_SUB_AcbHLb, Z_SUB_AcA     , Z_SBC_AcB     , Z_SBC_AcC     , Z_SBC_AcD     , Z_SBC_AcE     , Z_SBC_AcH     , Z_SBC_AcL     , ZxD_SBC_AcbHLb, Z_SBC_AcA   
main_Ax_DD:     dd Z_AND_AcB     , Z_AND_AcC     , Z_AND_AcD     , Z_AND_AcE     , Z_AND_AcH     , Z_AND_AcL     , ZxD_AND_AcbHLb, Z_AND_AcA     , Z_XOR_AcB     , Z_XOR_AcC     , Z_XOR_AcD     , Z_XOR_AcE     , Z_XOR_AcH     , Z_XOR_AcL     , ZxD_XOR_AcbHLb, Z_XOR_AcA   
main_Bx_DD:     dd Z_OR_AcB      , Z_OR_AcC      , Z_OR_AcD      , Z_OR_AcE      , Z_OR_AcH      , Z_OR_AcL      , ZxD_OR_AcbHLb , Z_OR_AcA      , Z_CP_AcB      , Z_CP_AcC      , Z_CP_AcD      , Z_CP_AcE      , Z_CP_AcH      , Z_CP_AcL      , ZxD_CP_AcbHLb , Z_CP_AcA    
main_Cx_DD:     dd Z_RET_NZ      , Z_POP_BC      , Z_JP_NZcnn    , Z_JP_nn       , Z_CALL_NZcnn  , Z_PUSH_BC     , Z_ADD_Acn     , Z_RST_00H     , Z_RET_Z       , Z_RET         , Z_JP_Zcnn     , ZDD_CB_PREF   , Z_CALL_Zcnn   , Z_CALL_nn     , Z_ADC_Acn     , Z_RST_08H   
main_Dx_DD:     dd Z_RET_NC      , Z_POP_DE      , Z_JP_NCcnn    , Z_OUT_bAnbcA  , Z_CALL_NCcnn  , Z_PUSH_DE     , Z_SUB_Acn     , Z_RST_10H     , Z_RET_C       , ZDD_EXX       , Z_JP_Ccnn     , Z_IN_AcbAnb   , Z_CALL_Ccnn   , ZDD_DD_PREF   , Z_SBC_Acn     , Z_RST_18H   
main_Ex_DD:     dd Z_RET_PO      , Z_POP_HL      , Z_JP_POcnn    , Z_EX_bSPbcHL  , Z_CALL_POcnn  , Z_PUSH_HL     , Z_AND_Acn     , Z_RST_20H     , Z_RET_PE      , Z_JP_HL       , Z_JP_PEcnn    , ZDD_EX_DEcHL  , Z_CALL_PEcnn  , ZDD_ED_PREF   , Z_XOR_Acn     , Z_RST_28H   
main_Fx_DD:     dd Z_RET_P       , Z_POP_AF      , Z_JP_Pcnn     , ZDD_DI        , Z_CALL_Pcnn   , Z_PUSH_AF     , Z_OR_Acn      , Z_RST_30H     , Z_RET_M       , Z_LD_SPcHL    , Z_JP_Mcnn     , ZDD_EI        , Z_CALL_Mcnn   , ZDD_FD_PREF   , Z_CP_Acn      , Z_RST_38H

main_opt_FD:
main_0x_FD:     dd Z_NOP         , Z_LD_BCcnn    , Z_LD_bBCbcA   , Z_INC_BC      , Z_INC_B       , Z_DEC_B       , Z_LD_Bcn      , Z_RLCA        , Z_EX_AFcAFp   , Z_ADD_HLcBC   , Z_LD_AcbBCb   , Z_DEC_BC      , Z_INC_C       , Z_DEC_C       , Z_LD_Ccn      , Z_RRCA      
main_1x_FD:     dd Z_DJNZ_e      , Z_LD_DEcnn    , Z_LD_bDEbcA   , Z_INC_DE      , Z_INC_D       , Z_DEC_D       , Z_LD_Dcn      , Z_RLA         , Z_JR_e        , Z_ADD_HLcDE   , Z_LD_AcbDEb   , Z_DEC_DE      , Z_INC_E       , Z_DEC_E       , Z_LD_Ecn      , Z_RRA       
main_2x_FD:     dd Z_JR_NZce     , Z_LD_HLcnn    , Z_LD_bnnbcHL  , Z_INC_HL      , Z_INC_H       , Z_DEC_H       , Z_LD_Hcn      , Z_DAA         , Z_JR_Zce      , Z_ADD_HLcHL   , Z_LD_HLcbnnb  , Z_DEC_HL      , Z_INC_L       , Z_DEC_L       , Z_LD_Lcn      , Z_CPL       
main_3x_FD:     dd Z_JR_NCce     , Z_LD_SPcnn    , Z_LD_bnnbcA   , Z_INC_SP      , ZxD_INC_bHLb  , ZxD_DEC_bHLb  , ZxD_LD_bHLbcn , Z_SCF         , Z_JR_Cce      , Z_ADD_HLcSP   , Z_LD_Acbnnb   , Z_DEC_SP      , Z_INC_A       , Z_DEC_A       , Z_LD_Acn      , Z_CCF       
main_4x_FD:     dd Z_LD_BcB      , Z_LD_BcC      , Z_LD_BcD      , Z_LD_BcE      , Z_LD_BcH      , Z_LD_BcL      , ZxD_LD_BcbHLb , Z_LD_BcA      , Z_LD_CcB      , Z_LD_CcC      , Z_LD_CcD      , Z_LD_CcE      , Z_LD_CcH      , Z_LD_CcL      , ZxD_LD_CcbHLb , Z_LD_CcA    
main_5x_FD:     dd Z_LD_DcB      , Z_LD_DcC      , Z_LD_DcD      , Z_LD_DcE      , Z_LD_DcH      , Z_LD_DcL      , ZxD_LD_DcbHLb , Z_LD_DcA      , Z_LD_EcB      , Z_LD_EcC      , Z_LD_EcD      , Z_LD_EcE      , Z_LD_EcH      , Z_LD_EcL      , ZxD_LD_EcbHLb , Z_LD_EcA    
main_6x_FD:     dd Z_LD_HcB      , Z_LD_HcC      , Z_LD_HcD      , Z_LD_HcE      , Z_LD_HcH      , Z_LD_HcL      , ZFD_LD_HcbHLb , Z_LD_HcA      , Z_LD_LcB      , Z_LD_LcC      , Z_LD_LcD      , Z_LD_LcE      , Z_LD_LcH      , Z_LD_LcL      , ZFD_LD_LcbHLb , Z_LD_LcA    
main_7x_FD:     dd ZxD_LD_bHLbcB , ZxD_LD_bHLbcC , ZxD_LD_bHLbcD , ZxD_LD_bHLbcE , ZFD_LD_bHLbcH , ZFD_LD_bHLbcL , ZFD_HALT      , ZxD_LD_bHLbcA , Z_LD_AcB      , Z_LD_AcC      , Z_LD_AcD      , Z_LD_AcE      , Z_LD_AcH      , Z_LD_AcL      , ZxD_LD_AcbHLb , Z_LD_AcA    
main_8x_FD:     dd Z_ADD_AcB     , Z_ADD_AcC     , Z_ADD_AcD     , Z_ADD_AcE     , Z_ADD_AcH     , Z_ADD_AcL     , ZxD_ADD_AcbHLb, Z_ADD_AcA     , Z_ADC_AcB     , Z_ADC_AcC     , Z_ADC_AcD     , Z_ADC_AcE     , Z_ADC_AcH     , Z_ADC_AcL     , ZxD_ADC_AcbHLb, Z_ADC_AcA   
main_9x_FD:     dd Z_SUB_AcB     , Z_SUB_AcC     , Z_SUB_AcD     , Z_SUB_AcE     , Z_SUB_AcH     , Z_SUB_AcL     , ZxD_SUB_AcbHLb, Z_SUB_AcA     , Z_SBC_AcB     , Z_SBC_AcC     , Z_SBC_AcD     , Z_SBC_AcE     , Z_SBC_AcH     , Z_SBC_AcL     , ZxD_SBC_AcbHLb, Z_SBC_AcA   
main_Ax_FD:     dd Z_AND_AcB     , Z_AND_AcC     , Z_AND_AcD     , Z_AND_AcE     , Z_AND_AcH     , Z_AND_AcL     , ZxD_AND_AcbHLb, Z_AND_AcA     , Z_XOR_AcB     , Z_XOR_AcC     , Z_XOR_AcD     , Z_XOR_AcE     , Z_XOR_AcH     , Z_XOR_AcL     , ZxD_XOR_AcbHLb, Z_XOR_AcA   
main_Bx_FD:     dd Z_OR_AcB      , Z_OR_AcC      , Z_OR_AcD      , Z_OR_AcE      , Z_OR_AcH      , Z_OR_AcL      , ZxD_OR_AcbHLb , Z_OR_AcA      , Z_CP_AcB      , Z_CP_AcC      , Z_CP_AcD      , Z_CP_AcE      , Z_CP_AcH      , Z_CP_AcL      , ZxD_CP_AcbHLb , Z_CP_AcA    
main_Cx_FD:     dd Z_RET_NZ      , Z_POP_BC      , Z_JP_NZcnn    , Z_JP_nn       , Z_CALL_NZcnn  , Z_PUSH_BC     , Z_ADD_Acn     , Z_RST_00H     , Z_RET_Z       , Z_RET         , Z_JP_Zcnn     , ZFD_CB_PREF   , Z_CALL_Zcnn   , Z_CALL_nn     , Z_ADC_Acn     , Z_RST_08H   
main_Dx_FD:     dd Z_RET_NC      , Z_POP_DE      , Z_JP_NCcnn    , Z_OUT_bAnbcA  , Z_CALL_NCcnn  , Z_PUSH_DE     , Z_SUB_Acn     , Z_RST_10H     , Z_RET_C       , ZFD_EXX       , Z_JP_Ccnn     , Z_IN_AcbAnb   , Z_CALL_Ccnn   , ZFD_DD_PREF   , Z_SBC_Acn     , Z_RST_18H   
main_Ex_FD:     dd Z_RET_PO      , Z_POP_HL      , Z_JP_POcnn    , Z_EX_bSPbcHL  , Z_CALL_POcnn  , Z_PUSH_HL     , Z_AND_Acn     , Z_RST_20H     , Z_RET_PE      , Z_JP_HL       , Z_JP_PEcnn    , ZFD_EX_DEcHL  , Z_CALL_PEcnn  , ZFD_ED_PREF   , Z_XOR_Acn     , Z_RST_28H   
main_Fx_FD:     dd Z_RET_P       , Z_POP_AF      , Z_JP_Pcnn     , ZFD_DI        , Z_CALL_Pcnn   , Z_PUSH_AF     , Z_OR_Acn      , Z_RST_30H     , Z_RET_M       , Z_LD_SPcHL    , Z_JP_Mcnn     , ZFD_EI        , Z_CALL_Mcnn   , ZFD_FD_PREF   , Z_CP_Acn      , Z_RST_38H

; Jump table for CB op subset

                ALIGN   4,db 0

CB_opt:
CB_0x:          dd ZCB_RLC_B      , ZCB_RLC_C      , ZCB_RLC_D      , ZCB_RLC_E      , ZCB_RLC_H      , ZCB_RLC_L      , ZCB_RLC_bHLb   , ZCB_RLC_A      , ZCB_RRC_B      , ZCB_RRC_C      , ZCB_RRC_D      , ZCB_RRC_E      , ZCB_RRC_H      , ZCB_RRC_L      , ZCB_RRC_bHLb   , ZCB_RRC_A      ,
CB_1x:          dd ZCB_RL_B       , ZCB_RL_C       , ZCB_RL_D       , ZCB_RL_E       , ZCB_RL_H       , ZCB_RL_L       , ZCB_RL_bHLb    , ZCB_RL_A       , ZCB_RR_B       , ZCB_RR_C       , ZCB_RR_D       , ZCB_RR_E       , ZCB_RR_H       , ZCB_RR_L       , ZCB_RR_bHLb    , ZCB_RR_A       ,
CB_2x:          dd ZCB_SLA_B      , ZCB_SLA_C      , ZCB_SLA_D      , ZCB_SLA_E      , ZCB_SLA_H      , ZCB_SLA_L      , ZCB_SLA_bHLb   , ZCB_SLA_A      , ZCB_SRA_B      , ZCB_SRA_C      , ZCB_SRA_D      , ZCB_SRA_E      , ZCB_SRA_H      , ZCB_SRA_L      , ZCB_SRA_bHLb   , ZCB_SRA_A      ,
CB_3x:          dd ZCB_SLL_B      , ZCB_SLL_C      , ZCB_SLL_D      , ZCB_SLL_E      , ZCB_SLL_H      , ZCB_SLL_L      , ZCB_SLL_bHLb   , ZCB_SLL_A      , ZCB_SRL_B      , ZCB_SRL_C      , ZCB_SRL_D      , ZCB_SRL_E      , ZCB_SRL_H      , ZCB_SRL_L      , ZCB_SRL_bHLb   , ZCB_SRL_A      ,
CB_4x:          dd ZCB_BIT_Bc0    , ZCB_BIT_Cc0    , ZCB_BIT_Dc0    , ZCB_BIT_Ec0    , ZCB_BIT_Hc0    , ZCB_BIT_Lc0    , ZCB_BIT_bHLbc0 , ZCB_BIT_Ac0    , ZCB_BIT_Bc1    , ZCB_BIT_Cc1    , ZCB_BIT_Dc1    , ZCB_BIT_Ec1    , ZCB_BIT_Hc1    , ZCB_BIT_Lc1    , ZCB_BIT_bHLbc1 , ZCB_BIT_Ac1    ,
CB_5x:          dd ZCB_BIT_Bc2    , ZCB_BIT_Cc2    , ZCB_BIT_Dc2    , ZCB_BIT_Ec2    , ZCB_BIT_Hc2    , ZCB_BIT_Lc2    , ZCB_BIT_bHLbc2 , ZCB_BIT_Ac2    , ZCB_BIT_Bc3    , ZCB_BIT_Cc3    , ZCB_BIT_Dc3    , ZCB_BIT_Ec3    , ZCB_BIT_Hc3    , ZCB_BIT_Lc3    , ZCB_BIT_bHLbc3 , ZCB_BIT_Ac3    ,
CB_6x:          dd ZCB_BIT_Bc4    , ZCB_BIT_Cc4    , ZCB_BIT_Dc4    , ZCB_BIT_Ec4    , ZCB_BIT_Hc4    , ZCB_BIT_Lc4    , ZCB_BIT_bHLbc4 , ZCB_BIT_Ac4    , ZCB_BIT_Bc5    , ZCB_BIT_Cc5    , ZCB_BIT_Dc5    , ZCB_BIT_Ec5    , ZCB_BIT_Hc5    , ZCB_BIT_Lc5    , ZCB_BIT_bHLbc5 , ZCB_BIT_Ac5    ,
CB_7x:          dd ZCB_BIT_Bc6    , ZCB_BIT_Cc6    , ZCB_BIT_Dc6    , ZCB_BIT_Ec6    , ZCB_BIT_Hc6    , ZCB_BIT_Lc6    , ZCB_BIT_bHLbc6 , ZCB_BIT_Ac6    , ZCB_BIT_Bc7    , ZCB_BIT_Cc7    , ZCB_BIT_Dc7    , ZCB_BIT_Ec7    , ZCB_BIT_Hc7    , ZCB_BIT_Lc7    , ZCB_BIT_bHLbc7 , ZCB_BIT_Ac7    ,
CB_8x:          dd ZCB_RES_Bc0    , ZCB_RES_Cc0    , ZCB_RES_Dc0    , ZCB_RES_Ec0    , ZCB_RES_Hc0    , ZCB_RES_Lc0    , ZCB_RES_bHLbc0 , ZCB_RES_Ac0    , ZCB_RES_Bc1    , ZCB_RES_Cc1    , ZCB_RES_Dc1    , ZCB_RES_Ec1    , ZCB_RES_Hc1    , ZCB_RES_Lc1    , ZCB_RES_bHLbc1 , ZCB_RES_Ac1    ,
CB_9x:          dd ZCB_RES_Bc2    , ZCB_RES_Cc2    , ZCB_RES_Dc2    , ZCB_RES_Ec2    , ZCB_RES_Hc2    , ZCB_RES_Lc2    , ZCB_RES_bHLbc2 , ZCB_RES_Ac2    , ZCB_RES_Bc3    , ZCB_RES_Cc3    , ZCB_RES_Dc3    , ZCB_RES_Ec3    , ZCB_RES_Hc3    , ZCB_RES_Lc3    , ZCB_RES_bHLbc3 , ZCB_RES_Ac3    ,
CB_Ax:          dd ZCB_RES_Bc4    , ZCB_RES_Cc4    , ZCB_RES_Dc4    , ZCB_RES_Ec4    , ZCB_RES_Hc4    , ZCB_RES_Lc4    , ZCB_RES_bHLbc4 , ZCB_RES_Ac4    , ZCB_RES_Bc5    , ZCB_RES_Cc5    , ZCB_RES_Dc5    , ZCB_RES_Ec5    , ZCB_RES_Hc5    , ZCB_RES_Lc5    , ZCB_RES_bHLbc5 , ZCB_RES_Ac5    ,
CB_Bx:          dd ZCB_RES_Bc6    , ZCB_RES_Cc6    , ZCB_RES_Dc6    , ZCB_RES_Ec6    , ZCB_RES_Hc6    , ZCB_RES_Lc6    , ZCB_RES_bHLbc6 , ZCB_RES_Ac6    , ZCB_RES_Bc7    , ZCB_RES_Cc7    , ZCB_RES_Dc7    , ZCB_RES_Ec7    , ZCB_RES_Hc7    , ZCB_RES_Lc7    , ZCB_RES_bHLbc7 , ZCB_RES_Ac7    ,
CB_Cx:          dd ZCB_SET_Bc0    , ZCB_SET_Cc0    , ZCB_SET_Dc0    , ZCB_SET_Ec0    , ZCB_SET_Hc0    , ZCB_SET_Lc0    , ZCB_SET_bHLbc0 , ZCB_SET_Ac0    , ZCB_SET_Bc1    , ZCB_SET_Cc1    , ZCB_SET_Dc1    , ZCB_SET_Ec1    , ZCB_SET_Hc1    , ZCB_SET_Lc1    , ZCB_SET_bHLbc1 , ZCB_SET_Ac1    ,
CB_Dx:          dd ZCB_SET_Bc2    , ZCB_SET_Cc2    , ZCB_SET_Dc2    , ZCB_SET_Ec2    , ZCB_SET_Hc2    , ZCB_SET_Lc2    , ZCB_SET_bHLbc2 , ZCB_SET_Ac2    , ZCB_SET_Bc3    , ZCB_SET_Cc3    , ZCB_SET_Dc3    , ZCB_SET_Ec3    , ZCB_SET_Hc3    , ZCB_SET_Lc3    , ZCB_SET_bHLbc3 , ZCB_SET_Ac3    ,
CB_Ex:          dd ZCB_SET_Bc4    , ZCB_SET_Cc4    , ZCB_SET_Dc4    , ZCB_SET_Ec4    , ZCB_SET_Hc4    , ZCB_SET_Lc4    , ZCB_SET_bHLbc4 , ZCB_SET_Ac4    , ZCB_SET_Bc5    , ZCB_SET_Cc5    , ZCB_SET_Dc5    , ZCB_SET_Ec5    , ZCB_SET_Hc5    , ZCB_SET_Lc5    , ZCB_SET_bHLbc5 , ZCB_SET_Ac5    ,
CB_Fx:          dd ZCB_SET_Bc6    , ZCB_SET_Cc6    , ZCB_SET_Dc6    , ZCB_SET_Ec6    , ZCB_SET_Hc6    , ZCB_SET_Lc6    , ZCB_SET_bHLbc6 , ZCB_SET_Ac6    , ZCB_SET_Bc7    , ZCB_SET_Cc7    , ZCB_SET_Dc7    , ZCB_SET_Ec7    , ZCB_SET_Hc7    , ZCB_SET_Lc7    , ZCB_SET_bHLbc7 , ZCB_SET_Ac7

CB_opt_DD:
CB_0x_DD:       dd ZxDCB_RLC_B      , ZxDCB_RLC_C      , ZxDCB_RLC_D      , ZxDCB_RLC_E      , ZDDCB_RLC_H      , ZDDCB_RLC_L      , ZxDCB_RLC_bHLb   , ZxDCB_RLC_A      , ZxDCB_RRC_B      , ZxDCB_RRC_C      , ZxDCB_RRC_D      , ZxDCB_RRC_E      , ZDDCB_RRC_H      , ZDDCB_RRC_L      , ZxDCB_RRC_bHLb   , ZxDCB_RRC_A      ,
CB_1x_DD:       dd ZxDCB_RL_B       , ZxDCB_RL_C       , ZxDCB_RL_D       , ZxDCB_RL_E       , ZDDCB_RL_H       , ZDDCB_RL_L       , ZxDCB_RL_bHLb    , ZxDCB_RL_A       , ZxDCB_RR_B       , ZxDCB_RR_C       , ZxDCB_RR_D       , ZxDCB_RR_E       , ZDDCB_RR_H       , ZDDCB_RR_L       , ZxDCB_RR_bHLb    , ZxDCB_RR_A       ,
CB_2x_DD:       dd ZxDCB_SLA_B      , ZxDCB_SLA_C      , ZxDCB_SLA_D      , ZxDCB_SLA_E      , ZDDCB_SLA_H      , ZDDCB_SLA_L      , ZxDCB_SLA_bHLb   , ZxDCB_SLA_A      , ZxDCB_SRA_B      , ZxDCB_SRA_C      , ZxDCB_SRA_D      , ZxDCB_SRA_E      , ZDDCB_SRA_H      , ZDDCB_SRA_L      , ZxDCB_SRA_bHLb   , ZxDCB_SRA_A      ,
CB_3x_DD:       dd ZxDCB_SLL_B      , ZxDCB_SLL_C      , ZxDCB_SLL_D      , ZxDCB_SLL_E      , ZDDCB_SLL_H      , ZDDCB_SLL_L      , ZxDCB_SLL_bHLb   , ZxDCB_SLL_A      , ZxDCB_SRL_B      , ZxDCB_SRL_C      , ZxDCB_SRL_D      , ZxDCB_SRL_E      , ZDDCB_SRL_H      , ZDDCB_SRL_L      , ZxDCB_SRL_bHLb   , ZxDCB_SRL_A      ,
CB_4x_DD:       dd ZxDCB_BIT_Bc0    , ZxDCB_BIT_Cc0    , ZxDCB_BIT_Dc0    , ZxDCB_BIT_Ec0    , ZDDCB_BIT_Hc0    , ZDDCB_BIT_Lc0    , ZxDCB_BIT_bHLbc0 , ZxDCB_BIT_Ac0    , ZxDCB_BIT_Bc1    , ZxDCB_BIT_Cc1    , ZxDCB_BIT_Dc1    , ZxDCB_BIT_Ec1    , ZDDCB_BIT_Hc1    , ZDDCB_BIT_Lc1    , ZxDCB_BIT_bHLbc1 , ZxDCB_BIT_Ac1    ,
CB_5x_DD:       dd ZxDCB_BIT_Bc2    , ZxDCB_BIT_Cc2    , ZxDCB_BIT_Dc2    , ZxDCB_BIT_Ec2    , ZDDCB_BIT_Hc2    , ZDDCB_BIT_Lc2    , ZxDCB_BIT_bHLbc2 , ZxDCB_BIT_Ac2    , ZxDCB_BIT_Bc3    , ZxDCB_BIT_Cc3    , ZxDCB_BIT_Dc3    , ZxDCB_BIT_Ec3    , ZDDCB_BIT_Hc3    , ZDDCB_BIT_Lc3    , ZxDCB_BIT_bHLbc3 , ZxDCB_BIT_Ac3    ,
CB_6x_DD:       dd ZxDCB_BIT_Bc4    , ZxDCB_BIT_Cc4    , ZxDCB_BIT_Dc4    , ZxDCB_BIT_Ec4    , ZDDCB_BIT_Hc4    , ZDDCB_BIT_Lc4    , ZxDCB_BIT_bHLbc4 , ZxDCB_BIT_Ac4    , ZxDCB_BIT_Bc5    , ZxDCB_BIT_Cc5    , ZxDCB_BIT_Dc5    , ZxDCB_BIT_Ec5    , ZDDCB_BIT_Hc5    , ZDDCB_BIT_Lc5    , ZxDCB_BIT_bHLbc5 , ZxDCB_BIT_Ac5    ,
CB_7x_DD:       dd ZxDCB_BIT_Bc6    , ZxDCB_BIT_Cc6    , ZxDCB_BIT_Dc6    , ZxDCB_BIT_Ec6    , ZDDCB_BIT_Hc6    , ZDDCB_BIT_Lc6    , ZxDCB_BIT_bHLbc6 , ZxDCB_BIT_Ac6    , ZxDCB_BIT_Bc7    , ZxDCB_BIT_Cc7    , ZxDCB_BIT_Dc7    , ZxDCB_BIT_Ec7    , ZDDCB_BIT_Hc7    , ZDDCB_BIT_Lc7    , ZxDCB_BIT_bHLbc7 , ZxDCB_BIT_Ac7    ,
CB_8x_DD:       dd ZxDCB_RES_Bc0    , ZxDCB_RES_Cc0    , ZxDCB_RES_Dc0    , ZxDCB_RES_Ec0    , ZDDCB_RES_Hc0    , ZDDCB_RES_Lc0    , ZxDCB_RES_bHLbc0 , ZxDCB_RES_Ac0    , ZxDCB_RES_Bc1    , ZxDCB_RES_Cc1    , ZxDCB_RES_Dc1    , ZxDCB_RES_Ec1    , ZDDCB_RES_Hc1    , ZDDCB_RES_Lc1    , ZxDCB_RES_bHLbc1 , ZxDCB_RES_Ac1    ,
CB_9x_DD:       dd ZxDCB_RES_Bc2    , ZxDCB_RES_Cc2    , ZxDCB_RES_Dc2    , ZxDCB_RES_Ec2    , ZDDCB_RES_Hc2    , ZDDCB_RES_Lc2    , ZxDCB_RES_bHLbc2 , ZxDCB_RES_Ac2    , ZxDCB_RES_Bc3    , ZxDCB_RES_Cc3    , ZxDCB_RES_Dc3    , ZxDCB_RES_Ec3    , ZDDCB_RES_Hc3    , ZDDCB_RES_Lc3    , ZxDCB_RES_bHLbc3 , ZxDCB_RES_Ac3    ,
CB_Ax_DD:       dd ZxDCB_RES_Bc4    , ZxDCB_RES_Cc4    , ZxDCB_RES_Dc4    , ZxDCB_RES_Ec4    , ZDDCB_RES_Hc4    , ZDDCB_RES_Lc4    , ZxDCB_RES_bHLbc4 , ZxDCB_RES_Ac4    , ZxDCB_RES_Bc5    , ZxDCB_RES_Cc5    , ZxDCB_RES_Dc5    , ZxDCB_RES_Ec5    , ZDDCB_RES_Hc5    , ZDDCB_RES_Lc5    , ZxDCB_RES_bHLbc5 , ZxDCB_RES_Ac5    ,
CB_Bx_DD:       dd ZxDCB_RES_Bc6    , ZxDCB_RES_Cc6    , ZxDCB_RES_Dc6    , ZxDCB_RES_Ec6    , ZDDCB_RES_Hc6    , ZDDCB_RES_Lc6    , ZxDCB_RES_bHLbc6 , ZxDCB_RES_Ac6    , ZxDCB_RES_Bc7    , ZxDCB_RES_Cc7    , ZxDCB_RES_Dc7    , ZxDCB_RES_Ec7    , ZDDCB_RES_Hc7    , ZDDCB_RES_Lc7    , ZxDCB_RES_bHLbc7 , ZxDCB_RES_Ac7    ,
CB_Cx_DD:       dd ZxDCB_SET_Bc0    , ZxDCB_SET_Cc0    , ZxDCB_SET_Dc0    , ZxDCB_SET_Ec0    , ZDDCB_SET_Hc0    , ZDDCB_SET_Lc0    , ZxDCB_SET_bHLbc0 , ZxDCB_SET_Ac0    , ZxDCB_SET_Bc1    , ZxDCB_SET_Cc1    , ZxDCB_SET_Dc1    , ZxDCB_SET_Ec1    , ZDDCB_SET_Hc1    , ZDDCB_SET_Lc1    , ZxDCB_SET_bHLbc1 , ZxDCB_SET_Ac1    ,
CB_Dx_DD:       dd ZxDCB_SET_Bc2    , ZxDCB_SET_Cc2    , ZxDCB_SET_Dc2    , ZxDCB_SET_Ec2    , ZDDCB_SET_Hc2    , ZDDCB_SET_Lc2    , ZxDCB_SET_bHLbc2 , ZxDCB_SET_Ac2    , ZxDCB_SET_Bc3    , ZxDCB_SET_Cc3    , ZxDCB_SET_Dc3    , ZxDCB_SET_Ec3    , ZDDCB_SET_Hc3    , ZDDCB_SET_Lc3    , ZxDCB_SET_bHLbc3 , ZxDCB_SET_Ac3    ,
CB_Ex_DD:       dd ZxDCB_SET_Bc4    , ZxDCB_SET_Cc4    , ZxDCB_SET_Dc4    , ZxDCB_SET_Ec4    , ZDDCB_SET_Hc4    , ZDDCB_SET_Lc4    , ZxDCB_SET_bHLbc4 , ZxDCB_SET_Ac4    , ZxDCB_SET_Bc5    , ZxDCB_SET_Cc5    , ZxDCB_SET_Dc5    , ZxDCB_SET_Ec5    , ZDDCB_SET_Hc5    , ZDDCB_SET_Lc5    , ZxDCB_SET_bHLbc5 , ZxDCB_SET_Ac5    ,
CB_Fx_DD:       dd ZxDCB_SET_Bc6    , ZxDCB_SET_Cc6    , ZxDCB_SET_Dc6    , ZxDCB_SET_Ec6    , ZDDCB_SET_Hc6    , ZDDCB_SET_Lc6    , ZxDCB_SET_bHLbc6 , ZxDCB_SET_Ac6    , ZxDCB_SET_Bc7    , ZxDCB_SET_Cc7    , ZxDCB_SET_Dc7    , ZxDCB_SET_Ec7    , ZDDCB_SET_Hc7    , ZDDCB_SET_Lc7    , ZxDCB_SET_bHLbc7 , ZxDCB_SET_Ac7

CB_opt_FD:
CB_0x_FD:       dd ZxDCB_RLC_B      , ZxDCB_RLC_C      , ZxDCB_RLC_D      , ZxDCB_RLC_E      , ZFDCB_RLC_H      , ZFDCB_RLC_L      , ZxDCB_RLC_bHLb   , ZxDCB_RLC_A      , ZxDCB_RRC_B      , ZxDCB_RRC_C      , ZxDCB_RRC_D      , ZxDCB_RRC_E      , ZFDCB_RRC_H      , ZFDCB_RRC_L      , ZxDCB_RRC_bHLb   , ZxDCB_RRC_A      ,
CB_1x_FD:       dd ZxDCB_RL_B       , ZxDCB_RL_C       , ZxDCB_RL_D       , ZxDCB_RL_E       , ZFDCB_RL_H       , ZFDCB_RL_L       , ZxDCB_RL_bHLb    , ZxDCB_RL_A       , ZxDCB_RR_B       , ZxDCB_RR_C       , ZxDCB_RR_D       , ZxDCB_RR_E       , ZFDCB_RR_H       , ZFDCB_RR_L       , ZxDCB_RR_bHLb    , ZxDCB_RR_A       ,
CB_2x_FD:       dd ZxDCB_SLA_B      , ZxDCB_SLA_C      , ZxDCB_SLA_D      , ZxDCB_SLA_E      , ZFDCB_SLA_H      , ZFDCB_SLA_L      , ZxDCB_SLA_bHLb   , ZxDCB_SLA_A      , ZxDCB_SRA_B      , ZxDCB_SRA_C      , ZxDCB_SRA_D      , ZxDCB_SRA_E      , ZFDCB_SRA_H      , ZFDCB_SRA_L      , ZxDCB_SRA_bHLb   , ZxDCB_SRA_A      ,
CB_3x_FD:       dd ZxDCB_SLL_B      , ZxDCB_SLL_C      , ZxDCB_SLL_D      , ZxDCB_SLL_E      , ZFDCB_SLL_H      , ZFDCB_SLL_L      , ZxDCB_SLL_bHLb   , ZxDCB_SLL_A      , ZxDCB_SRL_B      , ZxDCB_SRL_C      , ZxDCB_SRL_D      , ZxDCB_SRL_E      , ZFDCB_SRL_H      , ZFDCB_SRL_L      , ZxDCB_SRL_bHLb   , ZxDCB_SRL_A      ,
CB_4x_FD:       dd ZxDCB_BIT_Bc0    , ZxDCB_BIT_Cc0    , ZxDCB_BIT_Dc0    , ZxDCB_BIT_Ec0    , ZFDCB_BIT_Hc0    , ZFDCB_BIT_Lc0    , ZxDCB_BIT_bHLbc0 , ZxDCB_BIT_Ac0    , ZxDCB_BIT_Bc1    , ZxDCB_BIT_Cc1    , ZxDCB_BIT_Dc1    , ZxDCB_BIT_Ec1    , ZFDCB_BIT_Hc1    , ZFDCB_BIT_Lc1    , ZxDCB_BIT_bHLbc1 , ZxDCB_BIT_Ac1    ,
CB_5x_FD:       dd ZxDCB_BIT_Bc2    , ZxDCB_BIT_Cc2    , ZxDCB_BIT_Dc2    , ZxDCB_BIT_Ec2    , ZFDCB_BIT_Hc2    , ZFDCB_BIT_Lc2    , ZxDCB_BIT_bHLbc2 , ZxDCB_BIT_Ac2    , ZxDCB_BIT_Bc3    , ZxDCB_BIT_Cc3    , ZxDCB_BIT_Dc3    , ZxDCB_BIT_Ec3    , ZFDCB_BIT_Hc3    , ZFDCB_BIT_Lc3    , ZxDCB_BIT_bHLbc3 , ZxDCB_BIT_Ac3    ,
CB_6x_FD:       dd ZxDCB_BIT_Bc4    , ZxDCB_BIT_Cc4    , ZxDCB_BIT_Dc4    , ZxDCB_BIT_Ec4    , ZFDCB_BIT_Hc4    , ZFDCB_BIT_Lc4    , ZxDCB_BIT_bHLbc4 , ZxDCB_BIT_Ac4    , ZxDCB_BIT_Bc5    , ZxDCB_BIT_Cc5    , ZxDCB_BIT_Dc5    , ZxDCB_BIT_Ec5    , ZFDCB_BIT_Hc5    , ZFDCB_BIT_Lc5    , ZxDCB_BIT_bHLbc5 , ZxDCB_BIT_Ac5    ,
CB_7x_FD:       dd ZxDCB_BIT_Bc6    , ZxDCB_BIT_Cc6    , ZxDCB_BIT_Dc6    , ZxDCB_BIT_Ec6    , ZFDCB_BIT_Hc6    , ZFDCB_BIT_Lc6    , ZxDCB_BIT_bHLbc6 , ZxDCB_BIT_Ac6    , ZxDCB_BIT_Bc7    , ZxDCB_BIT_Cc7    , ZxDCB_BIT_Dc7    , ZxDCB_BIT_Ec7    , ZFDCB_BIT_Hc7    , ZFDCB_BIT_Lc7    , ZxDCB_BIT_bHLbc7 , ZxDCB_BIT_Ac7    ,
CB_8x_FD:       dd ZxDCB_RES_Bc0    , ZxDCB_RES_Cc0    , ZxDCB_RES_Dc0    , ZxDCB_RES_Ec0    , ZFDCB_RES_Hc0    , ZFDCB_RES_Lc0    , ZxDCB_RES_bHLbc0 , ZxDCB_RES_Ac0    , ZxDCB_RES_Bc1    , ZxDCB_RES_Cc1    , ZxDCB_RES_Dc1    , ZxDCB_RES_Ec1    , ZFDCB_RES_Hc1    , ZFDCB_RES_Lc1    , ZxDCB_RES_bHLbc1 , ZxDCB_RES_Ac1    ,
CB_9x_FD:       dd ZxDCB_RES_Bc2    , ZxDCB_RES_Cc2    , ZxDCB_RES_Dc2    , ZxDCB_RES_Ec2    , ZFDCB_RES_Hc2    , ZFDCB_RES_Lc2    , ZxDCB_RES_bHLbc2 , ZxDCB_RES_Ac2    , ZxDCB_RES_Bc3    , ZxDCB_RES_Cc3    , ZxDCB_RES_Dc3    , ZxDCB_RES_Ec3    , ZFDCB_RES_Hc3    , ZFDCB_RES_Lc3    , ZxDCB_RES_bHLbc3 , ZxDCB_RES_Ac3    ,
CB_Ax_FD:       dd ZxDCB_RES_Bc4    , ZxDCB_RES_Cc4    , ZxDCB_RES_Dc4    , ZxDCB_RES_Ec4    , ZFDCB_RES_Hc4    , ZFDCB_RES_Lc4    , ZxDCB_RES_bHLbc4 , ZxDCB_RES_Ac4    , ZxDCB_RES_Bc5    , ZxDCB_RES_Cc5    , ZxDCB_RES_Dc5    , ZxDCB_RES_Ec5    , ZFDCB_RES_Hc5    , ZFDCB_RES_Lc5    , ZxDCB_RES_bHLbc5 , ZxDCB_RES_Ac5    ,
CB_Bx_FD:       dd ZxDCB_RES_Bc6    , ZxDCB_RES_Cc6    , ZxDCB_RES_Dc6    , ZxDCB_RES_Ec6    , ZFDCB_RES_Hc6    , ZFDCB_RES_Lc6    , ZxDCB_RES_bHLbc6 , ZxDCB_RES_Ac6    , ZxDCB_RES_Bc7    , ZxDCB_RES_Cc7    , ZxDCB_RES_Dc7    , ZxDCB_RES_Ec7    , ZFDCB_RES_Hc7    , ZFDCB_RES_Lc7    , ZxDCB_RES_bHLbc7 , ZxDCB_RES_Ac7    ,
CB_Cx_FD:       dd ZxDCB_SET_Bc0    , ZxDCB_SET_Cc0    , ZxDCB_SET_Dc0    , ZxDCB_SET_Ec0    , ZFDCB_SET_Hc0    , ZFDCB_SET_Lc0    , ZxDCB_SET_bHLbc0 , ZxDCB_SET_Ac0    , ZxDCB_SET_Bc1    , ZxDCB_SET_Cc1    , ZxDCB_SET_Dc1    , ZxDCB_SET_Ec1    , ZFDCB_SET_Hc1    , ZFDCB_SET_Lc1    , ZxDCB_SET_bHLbc1 , ZxDCB_SET_Ac1    ,
CB_Dx_FD:       dd ZxDCB_SET_Bc2    , ZxDCB_SET_Cc2    , ZxDCB_SET_Dc2    , ZxDCB_SET_Ec2    , ZFDCB_SET_Hc2    , ZFDCB_SET_Lc2    , ZxDCB_SET_bHLbc2 , ZxDCB_SET_Ac2    , ZxDCB_SET_Bc3    , ZxDCB_SET_Cc3    , ZxDCB_SET_Dc3    , ZxDCB_SET_Ec3    , ZFDCB_SET_Hc3    , ZFDCB_SET_Lc3    , ZxDCB_SET_bHLbc3 , ZxDCB_SET_Ac3    ,
CB_Ex_FD:       dd ZxDCB_SET_Bc4    , ZxDCB_SET_Cc4    , ZxDCB_SET_Dc4    , ZxDCB_SET_Ec4    , ZFDCB_SET_Hc4    , ZFDCB_SET_Lc4    , ZxDCB_SET_bHLbc4 , ZxDCB_SET_Ac4    , ZxDCB_SET_Bc5    , ZxDCB_SET_Cc5    , ZxDCB_SET_Dc5    , ZxDCB_SET_Ec5    , ZFDCB_SET_Hc5    , ZFDCB_SET_Lc5    , ZxDCB_SET_bHLbc5 , ZxDCB_SET_Ac5    ,
CB_Fx_FD:       dd ZxDCB_SET_Bc6    , ZxDCB_SET_Cc6    , ZxDCB_SET_Dc6    , ZxDCB_SET_Ec6    , ZFDCB_SET_Hc6    , ZFDCB_SET_Lc6    , ZxDCB_SET_bHLbc6 , ZxDCB_SET_Ac6    , ZxDCB_SET_Bc7    , ZxDCB_SET_Cc7    , ZxDCB_SET_Dc7    , ZxDCB_SET_Ec7    , ZFDCB_SET_Hc7    , ZFDCB_SET_Lc7    , ZxDCB_SET_bHLbc7 , ZxDCB_SET_Ac7

; Jump table for ED op subset

                ALIGN   4,db 0

ED_opt:
ED_0x:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_1x:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_2x:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_3x:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_4x:          dd ZED_IN_BcbBCb  , ZED_OUT_bBCbB  , ZED_SBC_HLcBC  , ZED_LD_bnnbcBC , ZED_NEG        , ZED_RETIN      , ZED_IM_0       , ZED_LD_IcA     , ZED_IN_CcbBCb  , ZED_OUT_bBCbC  , ZED_ADC_HLcBC  , ZED_LD_BCcbnnb , ZED_NEG        , ZED_RETIN_REAL , ZED_IM_0       , ZED_LD_RcA     ,
ED_5x:          dd ZED_IN_DcbBCb  , ZED_OUT_bBCbD  , ZED_SBC_HLcDE  , ZED_LD_bnnbcDE , ZED_NEG        , ZED_RETIN      , ZED_IM_1       , ZED_LD_AcI     , ZED_IN_EcbBCb  , ZED_OUT_bBCbE  , ZED_ADC_HLcDE  , ZED_LD_DEcbnnb , ZED_NEG        , ZED_RETIN      , ZED_IM_2       , ZED_LD_AcR     ,
ED_6x:          dd ZED_IN_HcbBCb  , ZED_OUT_bBCbH  , ZED_SBC_HLcHL  , ZED_LD_bnnbcHL , ZED_NEG        , ZED_RETIN      , ZED_IM_0       , ZED_RRD        , ZED_IN_LcbBCb  , ZED_OUT_bBCbL  , ZED_ADC_HLcHL  , ZED_LD_HLcbnnb , ZED_NEG        , ZED_RETIN      , ZED_IM_0       , ZED_RLD        ,
ED_7x:          dd ZED_IN_bBCb    , ZED_OUT_bBCb0  , ZED_SBC_HLcSP  , ZED_LD_bnnbcSP , ZED_NEG        , ZED_RETIN      , ZED_IM_1       , ZED_NOP        , ZED_IN_AcbBCb  , ZED_OUT_bBCbA  , ZED_ADC_HLcSP  , ZED_LD_SPcbnnb , ZED_NEG        , ZED_RETIN      , ZED_IM_2       , ZED_NOP        ,
ED_8x:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_9x:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_Ax:          dd ZED_LDI        , ZED_CPI        , ZED_INI        , ZED_OUTI       , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_LDD        , ZED_CPD        , ZED_IND        , ZED_OUTD       , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_Bx:          dd ZED_LDIR       , ZED_CPIR       , ZED_INIR       , ZED_OUTIR      , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_LDDR       , ZED_CPDR       , ZED_INDR       , ZED_OUTDR      , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_Cx:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_Dx:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_Ex:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        ,
ED_Fx:          dd ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP        , ZED_NOP



; How we setup the start of the operation loop depends mostly on state
; byte 2.  This jump table goes to the correct loopstart proceedure for
; every possible state.
;
; low byte: state 2
; upper 2 bits: state 1 lsb (IFF1 and DI)

                ALIGN   4,db 0

;                  x0          , x1          , x2          , x3          , x4          , x5          , x6          , x7          , x8          , x9          , xA          , xB          , xC          , xD          , xE          , xF
state_jmp:
state_00x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_01x:      dd loop_nmi    , loop_intp   , loop_nmi_h  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_nmi    , loop_intp   , loop_nmi_h  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_02x:      dd loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_03x:      dd loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_04x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_05x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_06x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_07x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_08x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_09x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_0Ax:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_0Bx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_0Cx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_0Dx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_0Ex:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_0Fx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error

state_10x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_11x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_12x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_13x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_14x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_15x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_16x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_17x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_18x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_19x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_1Ax:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_1Bx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_1Cx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_1Dx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_1Ex:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_1Fx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error

state_20x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_int    , loop_intp   , loop_int_h  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_21x:      dd loop_nmi    , loop_intp   , loop_nmi_h  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_nmi    , loop_intp   , loop_nmi_h  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_22x:      dd loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_23x:      dd loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_brq    , loop_intp   , loop_brq    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_24x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_25x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_26x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_27x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_28x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_29x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_2Ax:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_2Bx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_2Cx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_2Dx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_2Ex:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_2Fx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error

state_30x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_31x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_32x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_33x:      dd loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_norm   , loop_intp   , loop_halt   , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_34x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_35x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_36x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_37x:      dd loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_res    , loop_res    , loop_res    , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_38x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_39x:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_3Ax:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_3Bx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_3Cx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_3Dx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_3Ex:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error
state_3Fx:      dd loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error  , loop_error



; DAA is implemented by lookup.
;
; The following lookup was generated by testing all possible A and F
; values on the z80 and recording the resultant A and F values.  Only
; H, N and C flags were found to affect result (as expected).  These are
; on the left (daaaHNC).
; So, to get result, make 11-bit HfNfCfA7A6A5A4A3A2A1A0 value and
; offset to start of relevant table (either daaa000 (A) or daaf000
; (F)).

; A lookup

                ALIGN   4,db 0

daaa000:
daaa000_0x: db 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015
daaa000_1x: db 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025
daaa000_2x: db 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035
daaa000_3x: db 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045
daaa000_4x: db 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055
daaa000_5x: db 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065
daaa000_6x: db 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075
daaa000_7x: db 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085
daaa000_8x: db 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089, 0x090, 0x091, 0x092, 0x093, 0x094, 0x095
daaa000_9x: db 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099, 0x000, 0x001, 0x002, 0x003, 0x004, 0x005
daaa000_ax: db 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015
daaa000_bx: db 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025
daaa000_cx: db 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035
daaa000_dx: db 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045
daaa000_ex: db 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055
daaa000_fx: db 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065

daaa001:
daaa001_0x: db 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075
daaa001_1x: db 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085
daaa001_2x: db 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089, 0x090, 0x091, 0x092, 0x093, 0x094, 0x095
daaa001_3x: db 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099, 0x0a0, 0x0a1, 0x0a2, 0x0a3, 0x0a4, 0x0a5
daaa001_4x: db 0x0a0, 0x0a1, 0x0a2, 0x0a3, 0x0a4, 0x0a5, 0x0a6, 0x0a7, 0x0a8, 0x0a9, 0x0b0, 0x0b1, 0x0b2, 0x0b3, 0x0b4, 0x0b5
daaa001_5x: db 0x0b0, 0x0b1, 0x0b2, 0x0b3, 0x0b4, 0x0b5, 0x0b6, 0x0b7, 0x0b8, 0x0b9, 0x0c0, 0x0c1, 0x0c2, 0x0c3, 0x0c4, 0x0c5
daaa001_6x: db 0x0c0, 0x0c1, 0x0c2, 0x0c3, 0x0c4, 0x0c5, 0x0c6, 0x0c7, 0x0c8, 0x0c9, 0x0d0, 0x0d1, 0x0d2, 0x0d3, 0x0d4, 0x0d5
daaa001_7x: db 0x0d0, 0x0d1, 0x0d2, 0x0d3, 0x0d4, 0x0d5, 0x0d6, 0x0d7, 0x0d8, 0x0d9, 0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e4, 0x0e5
daaa001_8x: db 0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e4, 0x0e5, 0x0e6, 0x0e7, 0x0e8, 0x0e9, 0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f4, 0x0f5
daaa001_9x: db 0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f4, 0x0f5, 0x0f6, 0x0f7, 0x0f8, 0x0f9, 0x000, 0x001, 0x002, 0x003, 0x004, 0x005
daaa001_ax: db 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015
daaa001_bx: db 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025
daaa001_cx: db 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035
daaa001_dx: db 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045
daaa001_ex: db 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055
daaa001_fx: db 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065

daaa010:
daaa010_0x: db 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009
daaa010_1x: db 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019
daaa010_2x: db 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029
daaa010_3x: db 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039
daaa010_4x: db 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049
daaa010_5x: db 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059
daaa010_6x: db 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069
daaa010_7x: db 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079
daaa010_8x: db 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089
daaa010_9x: db 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039
daaa010_ax: db 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049
daaa010_bx: db 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059
daaa010_cx: db 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069
daaa010_dx: db 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079
daaa010_ex: db 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089
daaa010_fx: db 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099

daaa011:
daaa011_0x: db 0x0a0, 0x0a1, 0x0a2, 0x0a3, 0x0a4, 0x0a5, 0x0a6, 0x0a7, 0x0a8, 0x0a9, 0x0a4, 0x0a5, 0x0a6, 0x0a7, 0x0a8, 0x0a9
daaa011_1x: db 0x0b0, 0x0b1, 0x0b2, 0x0b3, 0x0b4, 0x0b5, 0x0b6, 0x0b7, 0x0b8, 0x0b9, 0x0b4, 0x0b5, 0x0b6, 0x0b7, 0x0b8, 0x0b9
daaa011_2x: db 0x0c0, 0x0c1, 0x0c2, 0x0c3, 0x0c4, 0x0c5, 0x0c6, 0x0c7, 0x0c8, 0x0c9, 0x0c4, 0x0c5, 0x0c6, 0x0c7, 0x0c8, 0x0c9
daaa011_3x: db 0x0d0, 0x0d1, 0x0d2, 0x0d3, 0x0d4, 0x0d5, 0x0d6, 0x0d7, 0x0d8, 0x0d9, 0x0d4, 0x0d5, 0x0d6, 0x0d7, 0x0d8, 0x0d9
daaa011_4x: db 0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e4, 0x0e5, 0x0e6, 0x0e7, 0x0e8, 0x0e9, 0x0e4, 0x0e5, 0x0e6, 0x0e7, 0x0e8, 0x0e9
daaa011_5x: db 0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f4, 0x0f5, 0x0f6, 0x0f7, 0x0f8, 0x0f9, 0x0f4, 0x0f5, 0x0f6, 0x0f7, 0x0f8, 0x0f9
daaa011_6x: db 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009
daaa011_7x: db 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019
daaa011_8x: db 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029
daaa011_9x: db 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039
daaa011_ax: db 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049
daaa011_bx: db 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059
daaa011_cx: db 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069
daaa011_dx: db 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079
daaa011_ex: db 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089
daaa011_fx: db 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099

daaa100:
daaa100_0x: db 0x006, 0x007, 0x008, 0x009, 0x00a, 0x00b, 0x00c, 0x00d, 0x00e, 0x00f, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015
daaa100_1x: db 0x016, 0x017, 0x018, 0x019, 0x01a, 0x01b, 0x01c, 0x01d, 0x01e, 0x01f, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025
daaa100_2x: db 0x026, 0x027, 0x028, 0x029, 0x02a, 0x02b, 0x02c, 0x02d, 0x02e, 0x02f, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035
daaa100_3x: db 0x036, 0x037, 0x038, 0x039, 0x03a, 0x03b, 0x03c, 0x03d, 0x03e, 0x03f, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045
daaa100_4x: db 0x046, 0x047, 0x048, 0x049, 0x04a, 0x04b, 0x04c, 0x04d, 0x04e, 0x04f, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055
daaa100_5x: db 0x056, 0x057, 0x058, 0x059, 0x05a, 0x05b, 0x05c, 0x05d, 0x05e, 0x05f, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065
daaa100_6x: db 0x066, 0x067, 0x068, 0x069, 0x06a, 0x06b, 0x06c, 0x06d, 0x06e, 0x06f, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075
daaa100_7x: db 0x076, 0x077, 0x078, 0x079, 0x07a, 0x07b, 0x07c, 0x07d, 0x07e, 0x07f, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085
daaa100_8x: db 0x086, 0x087, 0x088, 0x089, 0x08a, 0x08b, 0x08c, 0x08d, 0x08e, 0x08f, 0x090, 0x091, 0x092, 0x093, 0x094, 0x095
daaa100_9x: db 0x096, 0x097, 0x098, 0x099, 0x09a, 0x09b, 0x09c, 0x09d, 0x09e, 0x09f, 0x000, 0x001, 0x002, 0x003, 0x004, 0x005
daaa100_ax: db 0x006, 0x007, 0x008, 0x009, 0x00a, 0x00b, 0x00c, 0x00d, 0x00e, 0x00f, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015
daaa100_bx: db 0x016, 0x017, 0x018, 0x019, 0x01a, 0x01b, 0x01c, 0x01d, 0x01e, 0x01f, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025
daaa100_cx: db 0x026, 0x027, 0x028, 0x029, 0x02a, 0x02b, 0x02c, 0x02d, 0x02e, 0x02f, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035
daaa100_dx: db 0x036, 0x037, 0x038, 0x039, 0x03a, 0x03b, 0x03c, 0x03d, 0x03e, 0x03f, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045
daaa100_ex: db 0x046, 0x047, 0x048, 0x049, 0x04a, 0x04b, 0x04c, 0x04d, 0x04e, 0x04f, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055
daaa100_fx: db 0x056, 0x057, 0x058, 0x059, 0x05a, 0x05b, 0x05c, 0x05d, 0x05e, 0x05f, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065

daaa101:
daaa101_0x: db 0x066, 0x067, 0x068, 0x069, 0x06a, 0x06b, 0x06c, 0x06d, 0x06e, 0x06f, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075
daaa101_1x: db 0x076, 0x077, 0x078, 0x079, 0x07a, 0x07b, 0x07c, 0x07d, 0x07e, 0x07f, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085
daaa101_2x: db 0x086, 0x087, 0x088, 0x089, 0x08a, 0x08b, 0x08c, 0x08d, 0x08e, 0x08f, 0x090, 0x091, 0x092, 0x093, 0x094, 0x095
daaa101_3x: db 0x096, 0x097, 0x098, 0x099, 0x09a, 0x09b, 0x09c, 0x09d, 0x09e, 0x09f, 0x0a0, 0x0a1, 0x0a2, 0x0a3, 0x0a4, 0x0a5
daaa101_4x: db 0x0a6, 0x0a7, 0x0a8, 0x0a9, 0x0aa, 0x0ab, 0x0ac, 0x0ad, 0x0ae, 0x0af, 0x0b0, 0x0b1, 0x0b2, 0x0b3, 0x0b4, 0x0b5
daaa101_5x: db 0x0b6, 0x0b7, 0x0b8, 0x0b9, 0x0ba, 0x0bb, 0x0bc, 0x0bd, 0x0be, 0x0bf, 0x0c0, 0x0c1, 0x0c2, 0x0c3, 0x0c4, 0x0c5
daaa101_6x: db 0x0c6, 0x0c7, 0x0c8, 0x0c9, 0x0ca, 0x0cb, 0x0cc, 0x0cd, 0x0ce, 0x0cf, 0x0d0, 0x0d1, 0x0d2, 0x0d3, 0x0d4, 0x0d5
daaa101_7x: db 0x0d6, 0x0d7, 0x0d8, 0x0d9, 0x0da, 0x0db, 0x0dc, 0x0dd, 0x0de, 0x0df, 0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e4, 0x0e5
daaa101_8x: db 0x0e6, 0x0e7, 0x0e8, 0x0e9, 0x0ea, 0x0eb, 0x0ec, 0x0ed, 0x0ee, 0x0ef, 0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f4, 0x0f5
daaa101_9x: db 0x0f6, 0x0f7, 0x0f8, 0x0f9, 0x0fa, 0x0fb, 0x0fc, 0x0fd, 0x0fe, 0x0ff, 0x000, 0x001, 0x002, 0x003, 0x004, 0x005
daaa101_ax: db 0x006, 0x007, 0x008, 0x009, 0x00a, 0x00b, 0x00c, 0x00d, 0x00e, 0x00f, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015
daaa101_bx: db 0x016, 0x017, 0x018, 0x019, 0x01a, 0x01b, 0x01c, 0x01d, 0x01e, 0x01f, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025
daaa101_cx: db 0x026, 0x027, 0x028, 0x029, 0x02a, 0x02b, 0x02c, 0x02d, 0x02e, 0x02f, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035
daaa101_dx: db 0x036, 0x037, 0x038, 0x039, 0x03a, 0x03b, 0x03c, 0x03d, 0x03e, 0x03f, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045
daaa101_ex: db 0x046, 0x047, 0x048, 0x049, 0x04a, 0x04b, 0x04c, 0x04d, 0x04e, 0x04f, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055
daaa101_fx: db 0x056, 0x057, 0x058, 0x059, 0x05a, 0x05b, 0x05c, 0x05d, 0x05e, 0x05f, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065

daaa110:
daaa110_0x: db 0x0fa, 0x0fb, 0x0fc, 0x0fd, 0x0fe, 0x0ff, 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009
daaa110_1x: db 0x00a, 0x00b, 0x00c, 0x00d, 0x00e, 0x00f, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019
daaa110_2x: db 0x01a, 0x01b, 0x01c, 0x01d, 0x01e, 0x01f, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029
daaa110_3x: db 0x02a, 0x02b, 0x02c, 0x02d, 0x02e, 0x02f, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039
daaa110_4x: db 0x03a, 0x03b, 0x03c, 0x03d, 0x03e, 0x03f, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049
daaa110_5x: db 0x04a, 0x04b, 0x04c, 0x04d, 0x04e, 0x04f, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059
daaa110_6x: db 0x05a, 0x05b, 0x05c, 0x05d, 0x05e, 0x05f, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069
daaa110_7x: db 0x06a, 0x06b, 0x06c, 0x06d, 0x06e, 0x06f, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079
daaa110_8x: db 0x07a, 0x07b, 0x07c, 0x07d, 0x07e, 0x07f, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089
daaa110_9x: db 0x08a, 0x08b, 0x08c, 0x08d, 0x08e, 0x08f, 0x090, 0x091, 0x092, 0x093, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039
daaa110_ax: db 0x03a, 0x03b, 0x03c, 0x03d, 0x03e, 0x03f, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049
daaa110_bx: db 0x04a, 0x04b, 0x04c, 0x04d, 0x04e, 0x04f, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059
daaa110_cx: db 0x05a, 0x05b, 0x05c, 0x05d, 0x05e, 0x05f, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069
daaa110_dx: db 0x06a, 0x06b, 0x06c, 0x06d, 0x06e, 0x06f, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079
daaa110_ex: db 0x07a, 0x07b, 0x07c, 0x07d, 0x07e, 0x07f, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089
daaa110_fx: db 0x08a, 0x08b, 0x08c, 0x08d, 0x08e, 0x08f, 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099

daaa111:
daaa111_0x: db 0x09a, 0x09b, 0x09c, 0x09d, 0x09e, 0x09f, 0x0a0, 0x0a1, 0x0a2, 0x0a3, 0x0a4, 0x0a5, 0x0a6, 0x0a7, 0x0a8, 0x0a9
daaa111_1x: db 0x0aa, 0x0ab, 0x0ac, 0x0ad, 0x0ae, 0x0af, 0x0b0, 0x0b1, 0x0b2, 0x0b3, 0x0b4, 0x0b5, 0x0b6, 0x0b7, 0x0b8, 0x0b9
daaa111_2x: db 0x0ba, 0x0bb, 0x0bc, 0x0bd, 0x0be, 0x0bf, 0x0c0, 0x0c1, 0x0c2, 0x0c3, 0x0c4, 0x0c5, 0x0c6, 0x0c7, 0x0c8, 0x0c9
daaa111_3x: db 0x0ca, 0x0cb, 0x0cc, 0x0cd, 0x0ce, 0x0cf, 0x0d0, 0x0d1, 0x0d2, 0x0d3, 0x0d4, 0x0d5, 0x0d6, 0x0d7, 0x0d8, 0x0d9
daaa111_4x: db 0x0da, 0x0db, 0x0dc, 0x0dd, 0x0de, 0x0df, 0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e4, 0x0e5, 0x0e6, 0x0e7, 0x0e8, 0x0e9
daaa111_5x: db 0x0ea, 0x0eb, 0x0ec, 0x0ed, 0x0ee, 0x0ef, 0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f4, 0x0f5, 0x0f6, 0x0f7, 0x0f8, 0x0f9
daaa111_6x: db 0x0fa, 0x0fb, 0x0fc, 0x0fd, 0x0fe, 0x0ff, 0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009
daaa111_7x: db 0x00a, 0x00b, 0x00c, 0x00d, 0x00e, 0x00f, 0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019
daaa111_8x: db 0x01a, 0x01b, 0x01c, 0x01d, 0x01e, 0x01f, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029
daaa111_9x: db 0x02a, 0x02b, 0x02c, 0x02d, 0x02e, 0x02f, 0x030, 0x031, 0x032, 0x033, 0x034, 0x035, 0x036, 0x037, 0x038, 0x039
daaa111_ax: db 0x03a, 0x03b, 0x03c, 0x03d, 0x03e, 0x03f, 0x040, 0x041, 0x042, 0x043, 0x044, 0x045, 0x046, 0x047, 0x048, 0x049
daaa111_bx: db 0x04a, 0x04b, 0x04c, 0x04d, 0x04e, 0x04f, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059
daaa111_cx: db 0x05a, 0x05b, 0x05c, 0x05d, 0x05e, 0x05f, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x067, 0x068, 0x069
daaa111_dx: db 0x06a, 0x06b, 0x06c, 0x06d, 0x06e, 0x06f, 0x070, 0x071, 0x072, 0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079
daaa111_ex: db 0x07a, 0x07b, 0x07c, 0x07d, 0x07e, 0x07f, 0x080, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086, 0x087, 0x088, 0x089
daaa111_fx: db 0x08a, 0x08b, 0x08c, 0x08d, 0x08e, 0x08f, 0x090, 0x091, 0x092, 0x093, 0x094, 0x095, 0x096, 0x097, 0x098, 0x099


; F lookup

                ALIGN   4,db 0

daaf000:
daaf000_0x: db 0x044, 0x000, 0x000, 0x004, 0x000, 0x004, 0x004, 0x000, 0x008, 0x00c, 0x010, 0x014, 0x014, 0x010, 0x014, 0x010
daaf000_1x: db 0x000, 0x004, 0x004, 0x000, 0x004, 0x000, 0x000, 0x004, 0x00c, 0x008, 0x030, 0x034, 0x034, 0x030, 0x034, 0x030
daaf000_2x: db 0x020, 0x024, 0x024, 0x020, 0x024, 0x020, 0x020, 0x024, 0x02c, 0x028, 0x034, 0x030, 0x030, 0x034, 0x030, 0x034
daaf000_3x: db 0x024, 0x020, 0x020, 0x024, 0x020, 0x024, 0x024, 0x020, 0x028, 0x02c, 0x010, 0x014, 0x014, 0x010, 0x014, 0x010
daaf000_4x: db 0x000, 0x004, 0x004, 0x000, 0x004, 0x000, 0x000, 0x004, 0x00c, 0x008, 0x014, 0x010, 0x010, 0x014, 0x010, 0x014
daaf000_5x: db 0x004, 0x000, 0x000, 0x004, 0x000, 0x004, 0x004, 0x000, 0x008, 0x00c, 0x034, 0x030, 0x030, 0x034, 0x030, 0x034
daaf000_6x: db 0x024, 0x020, 0x020, 0x024, 0x020, 0x024, 0x024, 0x020, 0x028, 0x02c, 0x030, 0x034, 0x034, 0x030, 0x034, 0x030
daaf000_7x: db 0x020, 0x024, 0x024, 0x020, 0x024, 0x020, 0x020, 0x024, 0x02c, 0x028, 0x090, 0x094, 0x094, 0x090, 0x094, 0x090
daaf000_8x: db 0x080, 0x084, 0x084, 0x080, 0x084, 0x080, 0x080, 0x084, 0x08c, 0x088, 0x094, 0x090, 0x090, 0x094, 0x090, 0x094
daaf000_9x: db 0x084, 0x080, 0x080, 0x084, 0x080, 0x084, 0x084, 0x080, 0x088, 0x08c, 0x055, 0x011, 0x011, 0x015, 0x011, 0x015
daaf000_ax: db 0x045, 0x001, 0x001, 0x005, 0x001, 0x005, 0x005, 0x001, 0x009, 0x00d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf000_bx: db 0x001, 0x005, 0x005, 0x001, 0x005, 0x001, 0x001, 0x005, 0x00d, 0x009, 0x031, 0x035, 0x035, 0x031, 0x035, 0x031
daaf000_cx: db 0x021, 0x025, 0x025, 0x021, 0x025, 0x021, 0x021, 0x025, 0x02d, 0x029, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035
daaf000_dx: db 0x025, 0x021, 0x021, 0x025, 0x021, 0x025, 0x025, 0x021, 0x029, 0x02d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf000_ex: db 0x001, 0x005, 0x005, 0x001, 0x005, 0x001, 0x001, 0x005, 0x00d, 0x009, 0x015, 0x011, 0x011, 0x015, 0x011, 0x015
daaf000_fx: db 0x005, 0x001, 0x001, 0x005, 0x001, 0x005, 0x005, 0x001, 0x009, 0x00d, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035

daaf001:
daaf001_0x: db 0x025, 0x021, 0x021, 0x025, 0x021, 0x025, 0x025, 0x021, 0x029, 0x02d, 0x031, 0x035, 0x035, 0x031, 0x035, 0x031
daaf001_1x: db 0x021, 0x025, 0x025, 0x021, 0x025, 0x021, 0x021, 0x025, 0x02d, 0x029, 0x091, 0x095, 0x095, 0x091, 0x095, 0x091
daaf001_2x: db 0x081, 0x085, 0x085, 0x081, 0x085, 0x081, 0x081, 0x085, 0x08d, 0x089, 0x095, 0x091, 0x091, 0x095, 0x091, 0x095
daaf001_3x: db 0x085, 0x081, 0x081, 0x085, 0x081, 0x085, 0x085, 0x081, 0x089, 0x08d, 0x0b5, 0x0b1, 0x0b1, 0x0b5, 0x0b1, 0x0b5
daaf001_4x: db 0x0a5, 0x0a1, 0x0a1, 0x0a5, 0x0a1, 0x0a5, 0x0a5, 0x0a1, 0x0a9, 0x0ad, 0x0b1, 0x0b5, 0x0b5, 0x0b1, 0x0b5, 0x0b1
daaf001_5x: db 0x0a1, 0x0a5, 0x0a5, 0x0a1, 0x0a5, 0x0a1, 0x0a1, 0x0a5, 0x0ad, 0x0a9, 0x095, 0x091, 0x091, 0x095, 0x091, 0x095
daaf001_6x: db 0x085, 0x081, 0x081, 0x085, 0x081, 0x085, 0x085, 0x081, 0x089, 0x08d, 0x091, 0x095, 0x095, 0x091, 0x095, 0x091
daaf001_7x: db 0x081, 0x085, 0x085, 0x081, 0x085, 0x081, 0x081, 0x085, 0x08d, 0x089, 0x0b1, 0x0b5, 0x0b5, 0x0b1, 0x0b5, 0x0b1
daaf001_8x: db 0x0a1, 0x0a5, 0x0a5, 0x0a1, 0x0a5, 0x0a1, 0x0a1, 0x0a5, 0x0ad, 0x0a9, 0x0b5, 0x0b1, 0x0b1, 0x0b5, 0x0b1, 0x0b5
daaf001_9x: db 0x0a5, 0x0a1, 0x0a1, 0x0a5, 0x0a1, 0x0a5, 0x0a5, 0x0a1, 0x0a9, 0x0ad, 0x055, 0x011, 0x011, 0x015, 0x011, 0x015
daaf001_ax: db 0x045, 0x001, 0x001, 0x005, 0x001, 0x005, 0x005, 0x001, 0x009, 0x00d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf001_bx: db 0x001, 0x005, 0x005, 0x001, 0x005, 0x001, 0x001, 0x005, 0x00d, 0x009, 0x031, 0x035, 0x035, 0x031, 0x035, 0x031
daaf001_cx: db 0x021, 0x025, 0x025, 0x021, 0x025, 0x021, 0x021, 0x025, 0x02d, 0x029, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035
daaf001_dx: db 0x025, 0x021, 0x021, 0x025, 0x021, 0x025, 0x025, 0x021, 0x029, 0x02d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf001_ex: db 0x001, 0x005, 0x005, 0x001, 0x005, 0x001, 0x001, 0x005, 0x00d, 0x009, 0x015, 0x011, 0x011, 0x015, 0x011, 0x015
daaf001_fx: db 0x005, 0x001, 0x001, 0x005, 0x001, 0x005, 0x005, 0x001, 0x009, 0x00d, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035

daaf010:
daaf010_0x: db 0x046, 0x002, 0x002, 0x006, 0x002, 0x006, 0x006, 0x002, 0x00a, 0x00e, 0x002, 0x006, 0x006, 0x002, 0x00a, 0x00e
daaf010_1x: db 0x002, 0x006, 0x006, 0x002, 0x006, 0x002, 0x002, 0x006, 0x00e, 0x00a, 0x006, 0x002, 0x002, 0x006, 0x00e, 0x00a
daaf010_2x: db 0x022, 0x026, 0x026, 0x022, 0x026, 0x022, 0x022, 0x026, 0x02e, 0x02a, 0x026, 0x022, 0x022, 0x026, 0x02e, 0x02a
daaf010_3x: db 0x026, 0x022, 0x022, 0x026, 0x022, 0x026, 0x026, 0x022, 0x02a, 0x02e, 0x022, 0x026, 0x026, 0x022, 0x02a, 0x02e
daaf010_4x: db 0x002, 0x006, 0x006, 0x002, 0x006, 0x002, 0x002, 0x006, 0x00e, 0x00a, 0x006, 0x002, 0x002, 0x006, 0x00e, 0x00a
daaf010_5x: db 0x006, 0x002, 0x002, 0x006, 0x002, 0x006, 0x006, 0x002, 0x00a, 0x00e, 0x002, 0x006, 0x006, 0x002, 0x00a, 0x00e
daaf010_6x: db 0x026, 0x022, 0x022, 0x026, 0x022, 0x026, 0x026, 0x022, 0x02a, 0x02e, 0x022, 0x026, 0x026, 0x022, 0x02a, 0x02e
daaf010_7x: db 0x022, 0x026, 0x026, 0x022, 0x026, 0x022, 0x022, 0x026, 0x02e, 0x02a, 0x026, 0x022, 0x022, 0x026, 0x02e, 0x02a
daaf010_8x: db 0x082, 0x086, 0x086, 0x082, 0x086, 0x082, 0x082, 0x086, 0x08e, 0x08a, 0x086, 0x082, 0x082, 0x086, 0x08e, 0x08a
daaf010_9x: db 0x086, 0x082, 0x082, 0x086, 0x082, 0x086, 0x086, 0x082, 0x08a, 0x08e, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf010_ax: db 0x003, 0x007, 0x007, 0x003, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b
daaf010_bx: db 0x007, 0x003, 0x003, 0x007, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f
daaf010_cx: db 0x027, 0x023, 0x023, 0x027, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf010_dx: db 0x023, 0x027, 0x027, 0x023, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b
daaf010_ex: db 0x083, 0x087, 0x087, 0x083, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b
daaf010_fx: db 0x087, 0x083, 0x083, 0x087, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f

daaf011:
daaf011_0x: db 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0ab, 0x0af, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0ab, 0x0af
daaf011_1x: db 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0af, 0x0ab, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0af, 0x0ab
daaf011_2x: db 0x087, 0x083, 0x083, 0x087, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f
daaf011_3x: db 0x083, 0x087, 0x087, 0x083, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b
daaf011_4x: db 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0af, 0x0ab, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0af, 0x0ab
daaf011_5x: db 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0ab, 0x0af, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0ab, 0x0af
daaf011_6x: db 0x047, 0x003, 0x003, 0x007, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f
daaf011_7x: db 0x003, 0x007, 0x007, 0x003, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b
daaf011_8x: db 0x023, 0x027, 0x027, 0x023, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b
daaf011_9x: db 0x027, 0x023, 0x023, 0x027, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf011_ax: db 0x003, 0x007, 0x007, 0x003, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b
daaf011_bx: db 0x007, 0x003, 0x003, 0x007, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f
daaf011_cx: db 0x027, 0x023, 0x023, 0x027, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf011_dx: db 0x023, 0x027, 0x027, 0x023, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b
daaf011_ex: db 0x083, 0x087, 0x087, 0x083, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b
daaf011_fx: db 0x087, 0x083, 0x083, 0x087, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f

daaf100:
daaf100_0x: db 0x004, 0x000, 0x008, 0x00c, 0x00c, 0x008, 0x00c, 0x008, 0x008, 0x00c, 0x010, 0x014, 0x014, 0x010, 0x014, 0x010
daaf100_1x: db 0x000, 0x004, 0x00c, 0x008, 0x008, 0x00c, 0x008, 0x00c, 0x00c, 0x008, 0x030, 0x034, 0x034, 0x030, 0x034, 0x030
daaf100_2x: db 0x020, 0x024, 0x02c, 0x028, 0x028, 0x02c, 0x028, 0x02c, 0x02c, 0x028, 0x034, 0x030, 0x030, 0x034, 0x030, 0x034
daaf100_3x: db 0x024, 0x020, 0x028, 0x02c, 0x02c, 0x028, 0x02c, 0x028, 0x028, 0x02c, 0x010, 0x014, 0x014, 0x010, 0x014, 0x010
daaf100_4x: db 0x000, 0x004, 0x00c, 0x008, 0x008, 0x00c, 0x008, 0x00c, 0x00c, 0x008, 0x014, 0x010, 0x010, 0x014, 0x010, 0x014
daaf100_5x: db 0x004, 0x000, 0x008, 0x00c, 0x00c, 0x008, 0x00c, 0x008, 0x008, 0x00c, 0x034, 0x030, 0x030, 0x034, 0x030, 0x034
daaf100_6x: db 0x024, 0x020, 0x028, 0x02c, 0x02c, 0x028, 0x02c, 0x028, 0x028, 0x02c, 0x030, 0x034, 0x034, 0x030, 0x034, 0x030
daaf100_7x: db 0x020, 0x024, 0x02c, 0x028, 0x028, 0x02c, 0x028, 0x02c, 0x02c, 0x028, 0x090, 0x094, 0x094, 0x090, 0x094, 0x090
daaf100_8x: db 0x080, 0x084, 0x08c, 0x088, 0x088, 0x08c, 0x088, 0x08c, 0x08c, 0x088, 0x094, 0x090, 0x090, 0x094, 0x090, 0x094
daaf100_9x: db 0x084, 0x080, 0x088, 0x08c, 0x08c, 0x088, 0x08c, 0x088, 0x088, 0x08c, 0x055, 0x011, 0x011, 0x015, 0x011, 0x015
daaf100_ax: db 0x005, 0x001, 0x009, 0x00d, 0x00d, 0x009, 0x00d, 0x009, 0x009, 0x00d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf100_bx: db 0x001, 0x005, 0x00d, 0x009, 0x009, 0x00d, 0x009, 0x00d, 0x00d, 0x009, 0x031, 0x035, 0x035, 0x031, 0x035, 0x031
daaf100_cx: db 0x021, 0x025, 0x02d, 0x029, 0x029, 0x02d, 0x029, 0x02d, 0x02d, 0x029, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035
daaf100_dx: db 0x025, 0x021, 0x029, 0x02d, 0x02d, 0x029, 0x02d, 0x029, 0x029, 0x02d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf100_ex: db 0x001, 0x005, 0x00d, 0x009, 0x009, 0x00d, 0x009, 0x00d, 0x00d, 0x009, 0x015, 0x011, 0x011, 0x015, 0x011, 0x015
daaf100_fx: db 0x005, 0x001, 0x009, 0x00d, 0x00d, 0x009, 0x00d, 0x009, 0x009, 0x00d, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035

daaf101:
daaf101_0x: db 0x025, 0x021, 0x029, 0x02d, 0x02d, 0x029, 0x02d, 0x029, 0x029, 0x02d, 0x031, 0x035, 0x035, 0x031, 0x035, 0x031
daaf101_1x: db 0x021, 0x025, 0x02d, 0x029, 0x029, 0x02d, 0x029, 0x02d, 0x02d, 0x029, 0x091, 0x095, 0x095, 0x091, 0x095, 0x091
daaf101_2x: db 0x081, 0x085, 0x08d, 0x089, 0x089, 0x08d, 0x089, 0x08d, 0x08d, 0x089, 0x095, 0x091, 0x091, 0x095, 0x091, 0x095
daaf101_3x: db 0x085, 0x081, 0x089, 0x08d, 0x08d, 0x089, 0x08d, 0x089, 0x089, 0x08d, 0x0b5, 0x0b1, 0x0b1, 0x0b5, 0x0b1, 0x0b5
daaf101_4x: db 0x0a5, 0x0a1, 0x0a9, 0x0ad, 0x0ad, 0x0a9, 0x0ad, 0x0a9, 0x0a9, 0x0ad, 0x0b1, 0x0b5, 0x0b5, 0x0b1, 0x0b5, 0x0b1
daaf101_5x: db 0x0a1, 0x0a5, 0x0ad, 0x0a9, 0x0a9, 0x0ad, 0x0a9, 0x0ad, 0x0ad, 0x0a9, 0x095, 0x091, 0x091, 0x095, 0x091, 0x095
daaf101_6x: db 0x085, 0x081, 0x089, 0x08d, 0x08d, 0x089, 0x08d, 0x089, 0x089, 0x08d, 0x091, 0x095, 0x095, 0x091, 0x095, 0x091
daaf101_7x: db 0x081, 0x085, 0x08d, 0x089, 0x089, 0x08d, 0x089, 0x08d, 0x08d, 0x089, 0x0b1, 0x0b5, 0x0b5, 0x0b1, 0x0b5, 0x0b1
daaf101_8x: db 0x0a1, 0x0a5, 0x0ad, 0x0a9, 0x0a9, 0x0ad, 0x0a9, 0x0ad, 0x0ad, 0x0a9, 0x0b5, 0x0b1, 0x0b1, 0x0b5, 0x0b1, 0x0b5
daaf101_9x: db 0x0a5, 0x0a1, 0x0a9, 0x0ad, 0x0ad, 0x0a9, 0x0ad, 0x0a9, 0x0a9, 0x0ad, 0x055, 0x011, 0x011, 0x015, 0x011, 0x015
daaf101_ax: db 0x005, 0x001, 0x009, 0x00d, 0x00d, 0x009, 0x00d, 0x009, 0x009, 0x00d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf101_bx: db 0x001, 0x005, 0x00d, 0x009, 0x009, 0x00d, 0x009, 0x00d, 0x00d, 0x009, 0x031, 0x035, 0x035, 0x031, 0x035, 0x031
daaf101_cx: db 0x021, 0x025, 0x02d, 0x029, 0x029, 0x02d, 0x029, 0x02d, 0x02d, 0x029, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035
daaf101_dx: db 0x025, 0x021, 0x029, 0x02d, 0x02d, 0x029, 0x02d, 0x029, 0x029, 0x02d, 0x011, 0x015, 0x015, 0x011, 0x015, 0x011
daaf101_ex: db 0x001, 0x005, 0x00d, 0x009, 0x009, 0x00d, 0x009, 0x00d, 0x00d, 0x009, 0x015, 0x011, 0x011, 0x015, 0x011, 0x015
daaf101_fx: db 0x005, 0x001, 0x009, 0x00d, 0x00d, 0x009, 0x00d, 0x009, 0x009, 0x00d, 0x035, 0x031, 0x031, 0x035, 0x031, 0x035

daaf110:
daaf110_0x: db 0x0be, 0x0ba, 0x0be, 0x0ba, 0x0ba, 0x0be, 0x046, 0x002, 0x002, 0x006, 0x002, 0x006, 0x006, 0x002, 0x00a, 0x00e
daaf110_1x: db 0x01e, 0x01a, 0x01e, 0x01a, 0x01a, 0x01e, 0x002, 0x006, 0x006, 0x002, 0x006, 0x002, 0x002, 0x006, 0x00e, 0x00a
daaf110_2x: db 0x01a, 0x01e, 0x01a, 0x01e, 0x01e, 0x01a, 0x022, 0x026, 0x026, 0x022, 0x026, 0x022, 0x022, 0x026, 0x02e, 0x02a
daaf110_3x: db 0x03a, 0x03e, 0x03a, 0x03e, 0x03e, 0x03a, 0x026, 0x022, 0x022, 0x026, 0x022, 0x026, 0x026, 0x022, 0x02a, 0x02e
daaf110_4x: db 0x03e, 0x03a, 0x03e, 0x03a, 0x03a, 0x03e, 0x002, 0x006, 0x006, 0x002, 0x006, 0x002, 0x002, 0x006, 0x00e, 0x00a
daaf110_5x: db 0x01a, 0x01e, 0x01a, 0x01e, 0x01e, 0x01a, 0x006, 0x002, 0x002, 0x006, 0x002, 0x006, 0x006, 0x002, 0x00a, 0x00e
daaf110_6x: db 0x01e, 0x01a, 0x01e, 0x01a, 0x01a, 0x01e, 0x026, 0x022, 0x022, 0x026, 0x022, 0x026, 0x026, 0x022, 0x02a, 0x02e
daaf110_7x: db 0x03e, 0x03a, 0x03e, 0x03a, 0x03a, 0x03e, 0x022, 0x026, 0x026, 0x022, 0x026, 0x022, 0x022, 0x026, 0x02e, 0x02a
daaf110_8x: db 0x03a, 0x03e, 0x03a, 0x03e, 0x03e, 0x03a, 0x082, 0x086, 0x086, 0x082, 0x086, 0x082, 0x082, 0x086, 0x08e, 0x08a
daaf110_9x: db 0x09a, 0x09e, 0x09a, 0x09e, 0x09e, 0x09a, 0x086, 0x082, 0x082, 0x086, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf110_ax: db 0x03f, 0x03b, 0x03f, 0x03b, 0x03b, 0x03f, 0x003, 0x007, 0x007, 0x003, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b
daaf110_bx: db 0x01b, 0x01f, 0x01b, 0x01f, 0x01f, 0x01b, 0x007, 0x003, 0x003, 0x007, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f
daaf110_cx: db 0x01f, 0x01b, 0x01f, 0x01b, 0x01b, 0x01f, 0x027, 0x023, 0x023, 0x027, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf110_dx: db 0x03f, 0x03b, 0x03f, 0x03b, 0x03b, 0x03f, 0x023, 0x027, 0x027, 0x023, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b
daaf110_ex: db 0x03b, 0x03f, 0x03b, 0x03f, 0x03f, 0x03b, 0x083, 0x087, 0x087, 0x083, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b
daaf110_fx: db 0x09b, 0x09f, 0x09b, 0x09f, 0x09f, 0x09b, 0x087, 0x083, 0x083, 0x087, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f

daaf111:
daaf111_0x: db 0x09f, 0x09b, 0x09f, 0x09b, 0x09b, 0x09f, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0ab, 0x0af
daaf111_1x: db 0x0bf, 0x0bb, 0x0bf, 0x0bb, 0x0bb, 0x0bf, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0af, 0x0ab
daaf111_2x: db 0x0bb, 0x0bf, 0x0bb, 0x0bf, 0x0bf, 0x0bb, 0x087, 0x083, 0x083, 0x087, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f
daaf111_3x: db 0x09f, 0x09b, 0x09f, 0x09b, 0x09b, 0x09f, 0x083, 0x087, 0x087, 0x083, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b
daaf111_4x: db 0x09b, 0x09f, 0x09b, 0x09f, 0x09f, 0x09b, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0af, 0x0ab
daaf111_5x: db 0x0bb, 0x0bf, 0x0bb, 0x0bf, 0x0bf, 0x0bb, 0x0a7, 0x0a3, 0x0a3, 0x0a7, 0x0a3, 0x0a7, 0x0a7, 0x0a3, 0x0ab, 0x0af
daaf111_6x: db 0x0bf, 0x0bb, 0x0bf, 0x0bb, 0x0bb, 0x0bf, 0x047, 0x003, 0x003, 0x007, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f
daaf111_7x: db 0x01f, 0x01b, 0x01f, 0x01b, 0x01b, 0x01f, 0x003, 0x007, 0x007, 0x003, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b
daaf111_8x: db 0x01b, 0x01f, 0x01b, 0x01f, 0x01f, 0x01b, 0x023, 0x027, 0x027, 0x023, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b
daaf111_9x: db 0x03b, 0x03f, 0x03b, 0x03f, 0x03f, 0x03b, 0x027, 0x023, 0x023, 0x027, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf111_ax: db 0x03f, 0x03b, 0x03f, 0x03b, 0x03b, 0x03f, 0x003, 0x007, 0x007, 0x003, 0x007, 0x003, 0x003, 0x007, 0x00f, 0x00b
daaf111_bx: db 0x01b, 0x01f, 0x01b, 0x01f, 0x01f, 0x01b, 0x007, 0x003, 0x003, 0x007, 0x003, 0x007, 0x007, 0x003, 0x00b, 0x00f
daaf111_cx: db 0x01f, 0x01b, 0x01f, 0x01b, 0x01b, 0x01f, 0x027, 0x023, 0x023, 0x027, 0x023, 0x027, 0x027, 0x023, 0x02b, 0x02f
daaf111_dx: db 0x03f, 0x03b, 0x03f, 0x03b, 0x03b, 0x03f, 0x023, 0x027, 0x027, 0x023, 0x027, 0x023, 0x023, 0x027, 0x02f, 0x02b
daaf111_ex: db 0x03b, 0x03f, 0x03b, 0x03f, 0x03f, 0x03b, 0x083, 0x087, 0x087, 0x083, 0x087, 0x083, 0x083, 0x087, 0x08f, 0x08b
daaf111_fx: db 0x09b, 0x09f, 0x09b, 0x09f, 0x09f, 0x09b, 0x087, 0x083, 0x083, 0x087, 0x083, 0x087, 0x087, 0x083, 0x08b, 0x08f

; Code used to find these (microbee basic on the 256tc)

; 00100 REM M/L SUBR: 123456789012345678901234567890
; 00110 FOR K=2320 TO 2332
; 00120   READ D
; 00130   POKE K,D
; 00140 NEXT K
; 00150 DATA 1,0,0,197,241,39,245,193,237,67,29,9,201
; 00160 OPEN"O",6,"DAA_SUM.DAT"
; 00170 OUTL#6
; 00180 FOR I = 0 TO 255
; 00190 FOR J = 0 TO 255
; 00200 POKE 2321,I
; 00210 POKE 2322,J
; 00220 K = USR(2320)
; 00230 F = PEEK(2333)
; 00240 A = PEEK(2334)
; 00250 LPRINT F;
; 00260 LPRINT" ";
; 00270 LPRINT A;
; 00280 LPRINT" ";
; 00290 NEXT J
; 00300 LPRINT
; 00310 PRINT I
; 00320 NEXT I
; 00330 OUTL#1
; 00340 CLOSE 6

; The assembler code on line 150 is:
;
; 01 00 00    ld bc,00       (lines 200/210 replace 00 with ij)
; c5          push bc
; f1          pop af
; 27          daa
; f5          push af
; c1          pop bc
; ed 43 1d 09 ld (091d),bc   (091d = 2333 - c in 2333, b in 2334)
; c9          ret


; NEG is implemented by lookup.
;
; The following lookup was generated by testing all possible A values
; on the z80 and recording the resultant A and F values.

; A lookup

                ALIGN   4,db 0

nega:
nega_0x: db 0x000, 0x0ff, 0x0fe, 0x0fd, 0x0fc, 0x0fb, 0x0fa, 0x0f9, 0x0f8, 0x0f7, 0x0f6, 0x0f5, 0x0f4, 0x0f3, 0x0f2, 0x0f1
nega_1x: db 0x0f0, 0x0ef, 0x0ee, 0x0ed, 0x0ec, 0x0eb, 0x0ea, 0x0e9, 0x0e8, 0x0e7, 0x0e6, 0x0e5, 0x0e4, 0x0e3, 0x0e2, 0x0e1
nega_2x: db 0x0e0, 0x0df, 0x0de, 0x0dd, 0x0dc, 0x0db, 0x0da, 0x0d9, 0x0d8, 0x0d7, 0x0d6, 0x0d5, 0x0d4, 0x0d3, 0x0d2, 0x0d1
nega_3x: db 0x0d0, 0x0cf, 0x0ce, 0x0cd, 0x0cc, 0x0cb, 0x0ca, 0x0c9, 0x0c8, 0x0c7, 0x0c6, 0x0c5, 0x0c4, 0x0c3, 0x0c2, 0x0c1
nega_4x: db 0x0c0, 0x0bf, 0x0be, 0x0bd, 0x0bc, 0x0bb, 0x0ba, 0x0b9, 0x0b8, 0x0b7, 0x0b6, 0x0b5, 0x0b4, 0x0b3, 0x0b2, 0x0b1
nega_5x: db 0x0b0, 0x0af, 0x0ae, 0x0ad, 0x0ac, 0x0ab, 0x0aa, 0x0a9, 0x0a8, 0x0a7, 0x0a6, 0x0a5, 0x0a4, 0x0a3, 0x0a2, 0x0a1
nega_6x: db 0x0a0, 0x09f, 0x09e, 0x09d, 0x09c, 0x09b, 0x09a, 0x099, 0x098, 0x097, 0x096, 0x095, 0x094, 0x093, 0x092, 0x091
nega_7x: db 0x090, 0x08f, 0x08e, 0x08d, 0x08c, 0x08b, 0x08a, 0x089, 0x088, 0x087, 0x086, 0x085, 0x084, 0x083, 0x082, 0x081
nega_8x: db 0x080, 0x07f, 0x07e, 0x07d, 0x07c, 0x07b, 0x07a, 0x079, 0x078, 0x077, 0x076, 0x075, 0x074, 0x073, 0x072, 0x071
nega_9x: db 0x070, 0x06f, 0x06e, 0x06d, 0x06c, 0x06b, 0x06a, 0x069, 0x068, 0x067, 0x066, 0x065, 0x064, 0x063, 0x062, 0x061
nega_ax: db 0x060, 0x05f, 0x05e, 0x05d, 0x05c, 0x05b, 0x05a, 0x059, 0x058, 0x057, 0x056, 0x055, 0x054, 0x053, 0x052, 0x051
nega_bx: db 0x050, 0x04f, 0x04e, 0x04d, 0x04c, 0x04b, 0x04a, 0x049, 0x048, 0x047, 0x046, 0x045, 0x044, 0x043, 0x042, 0x041
nega_cx: db 0x040, 0x03f, 0x03e, 0x03d, 0x03c, 0x03b, 0x03a, 0x039, 0x038, 0x037, 0x036, 0x035, 0x034, 0x033, 0x032, 0x031
nega_dx: db 0x030, 0x02f, 0x02e, 0x02d, 0x02c, 0x02b, 0x02a, 0x029, 0x028, 0x027, 0x026, 0x025, 0x024, 0x023, 0x022, 0x021
nega_ex: db 0x020, 0x01f, 0x01e, 0x01d, 0x01c, 0x01b, 0x01a, 0x019, 0x018, 0x017, 0x016, 0x015, 0x014, 0x013, 0x012, 0x011
nega_fx: db 0x010, 0x00f, 0x00e, 0x00d, 0x00c, 0x00b, 0x00a, 0x009, 0x008, 0x007, 0x006, 0x005, 0x004, 0x003, 0x002, 0x001


; F lookup

                ALIGN   4,db 0

negf:
negf_0x: db 0x042, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3
negf_1x: db 0x0a3, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3
negf_2x: db 0x0a3, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093
negf_3x: db 0x083, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093
negf_4x: db 0x083, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3
negf_5x: db 0x0a3, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0bb, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3, 0x0b3
negf_6x: db 0x0a3, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093
negf_7x: db 0x083, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x09b, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093, 0x093
negf_8x: db 0x087, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033
negf_9x: db 0x023, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033
negf_ax: db 0x023, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013
negf_bx: db 0x003, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013
negf_cx: db 0x003, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033
negf_dx: db 0x023, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x03b, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033
negf_ex: db 0x023, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013
negf_fx: db 0x003, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x01b, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013, 0x013

; Code used to find these (microbee basic on the 256tc)

; 00100 REM M/L SUBR: 123456789012345678901234567890
; 00110 FOR K=2320 TO 2334
; 00120   READ D
; 00130   POKE K,D
; 00140 NEXT K
; 00150 DATA 62,0,237,68,245,193,120,50,0,248,121,50,1,248,201
; 00160 OPEN"O",6,"NEG_SUM.DAT"
; 00170 OUTL#6
; 00180 FOR I = 0 TO 255
; 00190 POKE 2321,I
; 00200 K = USR(2320)
; 00210 A = PEEK(63488)
; 00220 LPRINT A
; 00230 NEXT I
; 00240 FOR I = 0 TO 255
; 00250 POKE 2321,I
; 00260 K = USR(2320)
; 00270 F = PEEK(63489)
; 00280 LPRINT F
; 00290 NEXT I
; 00300 OUTL#1
; 00310 CLOSE 6

; INI{R}/IND{R}/OUTI{R}/OUTD{R} temp2 LUT

                ALIGN   4,db 0

ioblock:
bhb0:   dw 1101001100101100b
bhb1:   dw 0010110011010010b
bhb2:   dw 0010110011010011b
bhb3:   dw 1101001100101101b
bhb4:   dw 0010110011010010b
bhb5:   dw 1101001100101101b
bhb6:   dw 1101001100101100b
bhb7:   dw 0010110011010010b
bhb8:   dw 0010110011010011b
bhb9:   dw 1101001100101101b
bhba:   dw 1101001100101100b
bhbb:   dw 0010110011010010b
bhbc:   dw 1101001100101101b
bhbd:   dw 0010110011010010b
bhbe:   dw 0010110011010011b
bhbf:   dw 1101001100101101b











































scratchpoint    dd      _z80_tab_num_bus

                SECTION .bss
                ALIGN   4,resb 1

;
; NB: The following structure describes how the cpu scratchpad is organised.
;     Note, however, that the actual memory here is not used - instead,
;     the external caller must provide a scratch pad of appropriate size
;     (0x01500) which allows multiple instances of cpu emulator to run
;     simultaneously.
;


;
; Z80 buses for setting memory/io controls
;

_z80_tab_num_bus:       resd    1       ; 1 byte
_z80_tab_addr_bus:      resd    1       ; 1 dword
_z80_tab_wr_wait_bus:   resd    1       ; 1 word
_z80_tab_rd_wait_bus:   resd    1       ; 1 word

;
; When the simulator calls external functions, it passes data to and from
; them using the following data buses.
;
; Note: wait bus and T-state counter are same thing.
;

_z80_wait_bus:          resd    1       ; 1 byte
_z80_rfsh_bus:          resd    1       ; 1 byte
_z80_data_bus:          resd    1       ; 1 byte
_z80_addr_bus:          resd    1       ; 1 word
unused_bus:             resd    1       ; (was used, kept for alignment)
_z80_clk_bus:           resd    1       ; 1 word

;
; RETI opcode counter
;

_z80_reti_count_bus:    resd    1       ; 1 dword

;
; Jump table selector
;
; what_ops = address of start of opcode table being used
; what_ret = address of return function being used
;
; what_res is necessary because in many cases the same function is used
; for all of normal, DD and FD variants of an opcode.
;

what_ops:               resd    1       ; 1 dword
what_ret:               resd    1       ; 1 dword

;
; Opfetch masking dword and PC increment
;
; op_mask = 0x0000000ff for normal opreads, 0x000000000 for HALT
; PC_incr = 0x000010000 for normal opreads, 0x000000000 for HALT
;

op_mask:                resd    1       ; 1 dword
PC_incr:                resd    1       ; 1 dword

;
; Opcode mem/io rd/wr function pointers
;
; getopbyte = function to get additional opcode bytes.  Will also increment
;             PC if relevant (ie. unless midway through INTfetch cycle).
;             Opcode read is placed in edi(l).
;             options: mem_rd_PC_inc, INT_rd_PC_inc
;
; NOTE: - functions should call getopbyte.
;       - when changing to memory ops, copy mem_rd_PC_inc over getopbyte
;       - when changing to interupt ops, copy INT_rd_PC_inc over getopbyte
;       - d = CB_d_off (offset, not straight add)
;

getopbyte:              resd    1       ; 1 dword

;
; Z80 CPU stuff not stored in x86 registers
;

loc_FAx:                resd    1       ; 1 word
loc_BCx:                resd    1       ; 1 word
loc_DEx:                resd    1       ; 1 word
loc_HLx:                resd    1       ; 1 word

;
; d offset must be saved when dealing with xDCB prefixed opcodes
;
; hidden_reg - this is needed to calc the undocced flags
; for the BIT (...) opcodes.  However, the only part of this that has been
; inplemented is the memory read from offset memory, so BIT (HL) is not
; implemented as well as could be.
;

CB_d_off:               resd    1       ; 1 byte
hidden_reg:             resd    1       ; 1 word

;
; 16-bit store for +d mem operations
;

xx_32_store:            resd    1       ; 1 dword

;
; Temporary storage
;

temp_32_store:          resd    1       ; 1 dword
temp_32_storeb:         resd    1       ; 1 dword
temp_16_store:          resd    1       ; 1 word
temp_8_store:           resd    1       ; 1 byte

;
; Asynch state/wait store
;

state_byte:             resd    1       ; 1 byte
wait_word:              resd    1       ; 1 word

;
; Storage area for cpu state during calls
;

local_eax:              resd    1       ; 1 dword
local_ebx:              resd    1       ; 1 dword
local_ecx:              resd    1       ; 1 dword
local_edx:              resd    1       ; 1 dword
local_esi:              resd    1       ; 1 dword
local_edi:              resd    1       ; 1 dword

;
; Memory operation control data
; =============================
;
; Memory is divided into 256 pages (ie. the upper 8 bytes of the address
; bus select the table).  The method used to access each table using read,
; write or opread is controlled using the following tables:
;
; mem_wtrd_table: 256 words  - tables of default waits on read / opread
; mem_wtwr_table: 256 words  - tables of default waits on write
; mem_addr_table: 256 dwords - addresses of 256*256 packed byte pages to be
;                              used as memory if direct access is enabled.
;
; memrd_jump_tble: read function, one of mrmemrd_call (if call to
;                  _z80_rd_mem is required), mrmemrd_mem (direct
;                  memory read from relevant table given by
;                  mem_addr_table) or mrmemrd_none if memory read
;                  returns 0.
; memwr_jump_tble: write function.  Like read, must be one of
;                  mrmemwr_call, mrmemwr_mem or mrmemwr_none.
; oprd_jump_tble:  opread function.  Like read, must be one of
;                  opfetch_call, opfetch_mem or opfetch_none.
;
;
; By default, all are set to call the relevant emulator function.  If other
; methods are required, these should be set before calling the z80 emulator
; using the relevant functions.
;
; NB. Memory reads from PC when dealing with IM0 will all call external
;     read function, regardless of setting here.  Also, wait states here
;     will be ignored in this case.
;

mem_wtwr_table:         resw    256     ; 256 words (128 dwords)
mem_wtrd_table:         resw    256     ; 256 words (128 dwords)
mem_addr_table:         resd    256     ; 256 dwords
oprd_jump_tble:         resd    256     ; 256 dwords
memrd_jump_tble:        resd    256     ; 256 dwords
memwr_jump_tble:        resd    256     ; 256 dwords

;
; Adjustable return address from "return".  Because there are a number of
; possible cpu states (halted, servicing interupt, normal etc.), calling
; sync_clock may need to jump to any number of different code sync points.
; To do this easily, the cpu emulator (this code) is made to imagine that
; it is "in control".  Therefore, when a sync point is reached, it will
; call call_switch (the stack state is always the same at a sync point).
; The function call_switch will pop the return address off the stack and
; store it in this variable.  It will then execute a retn operation which,
; as the stack state is unchanged from the last sync point, will return
; to the original (external) caller.  When the external caller next calls
; sync_clock, the address here will be pushed back onto the stack and a
; retn executed, returning execution to the relevant sync point.
;
; On initialisation, the this will be set to loop_st.
;

retaddr:                resd    1       ; 1 dword





;
; Dereferences for the above.
;

%define red_z80_tab_num_bus     ebp+0x00000
%define red_z80_tab_addr_bus    ebp+0x00004
%define red_z80_tab_wr_wait_bus ebp+0x00008
%define red_z80_tab_rd_wait_bus ebp+0x0000c
%define red_z80_wait_bus        ebp+0x00010
%define red_z80_rfsh_bus        ebp+0x00014
%define red_z80_data_bus        ebp+0x00018
%define red_z80_addr_bus        ebp+0x0001c
%define red_z80_clk_bus         ebp+0x00024
%define red_z80_reti_count_bus  ebp+0x00028
%define red_what_ops            ebp+0x0002c
%define red_what_ret            ebp+0x00030
%define red_op_mask             ebp+0x00034
%define red_PC_incr             ebp+0x00038
%define red_getopbyte           ebp+0x0003c
%define red_loc_FAx             ebp+0x00040
%define red_loc_BCx             ebp+0x00044
%define red_loc_DEx             ebp+0x00048
%define red_loc_HLx             ebp+0x0004c
%define red_CB_d_off            ebp+0x00050
%define red_hidden_reg          ebp+0x00054
%define red_xx_32_store         ebp+0x00058
%define red_temp_32_store       ebp+0x0005c
%define red_temp_32_storeb      ebp+0x00060
%define red_temp_16_store       ebp+0x00064
%define red_temp_8_store        ebp+0x00068
%define red_state_byte          ebp+0x0006c
%define red_wait_word           ebp+0x00070
%define red_local_eax           ebp+0x00074
%define red_local_ebx           ebp+0x00078
%define red_local_ecx           ebp+0x0007c
%define red_local_edx           ebp+0x00080
%define red_local_esi           ebp+0x00084
%define red_local_edi           ebp+0x00088
%define red_mem_wtwr_table      ebp+0x0008c
%define red_mem_wtrd_table      ebp+0x0028c
%define red_mem_addr_table      ebp+0x0048c
%define red_oprd_jump_tble      ebp+0x0088c
%define red_memrd_jump_tble     ebp+0x00c8c
%define red_memwr_jump_tble     ebp+0x0108c
%define red_retaddr             ebp+0x0148c






































;
; Register mapping: z80 -> x86
; ============================
;
;
;                                     ax
;                            /-------------------\
;       32                  16   ah        al    0
;        +-------------------+---------+---------+
; eax    | state 1 | state 2 |    F    |    A    |
;        +-------------------+---------+---------+
; ebx    |   PCh   |   PCl   |    B    |    C    |
;        +-------------------+---------+---------+
; ecx    |   SPh   |   SPl   |    D    |    E    |
;        +-------------------+---------+---------+
; edx    |    I    |    R    |    H    |    L    |
;        +-------------------+-------------------+
; esi    |   IYh   |   IYl   |   IXh   |   IXl   |
;        +-------------------+-------------------+
; edi    |      used as general purpose reg      |
;        +-------------------+-------------------+
; ebp    |    address of current scratch table   |
;        +-------------------+-------------------+
; esp    |***************************************|
;        +-------------------+-------------------+
;
;
; State byte 1
; ============
;
; 7                                                       0
; +------+------+------+------+------+------+------+------+
; |   -----mode-----   | IM1  | IM0  | IFF2 | IFF1 |  DI  |
; +------+------+------+------+------+------+------+------+
;
; mode: 000 = norm
;       001 = DD opmode
;       010 = FD opmode
;       100 = CB opmode
;       111 = ED opmode
;       101 = DDCB opmode
;       110 = FDCB opmode
;
;  IM0 IM1 | z80 int mode
; ---------+--------------
;   0   0  |    0
;   0   1  |    0
;   1   0  |    1
;   1   1  |    2
;
; IFF2/1 - internal z80 interupt state bits.  See z80 docs for details.
;
; DI - (disable interupts) set if last opcode was EI,DI or a prefix
;      (CB,DD,FD,ED).  If this bit is set then both NMI and INT signals will
;      be ignored (delayed until this is goes to 0 again).  In this way the
;      z80 is designed to not be interupted mid opcode (even though the
;      prefix bytes are in many ways treated as opcodes on their own).
;
;
; State byte 2
; ============
;
; 7                                                               0
; +-------+-------+-------+-------+-------+-------+-------+-------+
; |unused | RESET | BUSRQ |  NMI  |  INT  |unused |HALTED | INTOP |
; +-------+-------+-------+-------+-------+-------+-------+-------+
;
; HALTED - processor halted and doing NOPs until INT/NMI received
; INTOP  - interupt has been received, so pull op from appropriate memory
;          address using call regardless of memory mode.
;
; external: RESET - reset signal.  Set by z80_set_reset.  Reset immediately
;                   upon being serviced.
;           BUSRQ - bus request signal.  Set by z80_set_busrq.  Reset
;                   immediately upon being serviced.
;           NMI   - non-maskable interupt signal.  Set by z80_set_NMI.
;                   Reset immediately upon being serviced.
;           INT   - maskable interupt signal.  Set by z80_set_INT.  Reset
;                   manually by z80_res_INT (the state is persistent
;                   internally, as this signal is level sensitive on the
;                   z80 cpu, not edge triggered).
;












;***********************************************************************;
;Start code here.                                                       ;
;***********************************************************************;

                SECTION .text

;***********************************************************************;
;                                                                       ;
; Macros section:                                                       ;
;                                                                       ;
; push_all:             push everything onto the stack                  ;
; push_al_getscr:       push everything and point ebp to scratchpad     ;
; pop_all:              retrieve everything from the stack              ;
;                                                                       ;
;***********************************************************************;

%macro          push_all        0
                push    ebp
                push    edi
                push    esi
                push    edx
                push    ecx
                push    ebx
                push    eax
%endmacro

%macro          push_al_getscr  0
                push    ebp
;phantom
;                mov     ebp,[scratchpoint]
                mov     ebp,[esp+8]
;phantom
                push    edi
                push    esi
                push    edx
                push    ecx
                push    ebx
                push    eax
%endmacro

%macro          pop_all         0
                pop     eax
                pop     ebx
                pop     ecx
                pop     edx
                pop     esi
                pop     edi
                pop     ebp
%endmacro



;***********************************************************************;
;                                                                       ;
; Setup function section:                                               ;
;                                                                       ;
; _z80_init:            clear the parts of the z80 state that should    ;
;                       remain static during a normal reset.            ;
; clear_state:          clear the z80 state on startup.  This should be ;
;                       run at startup for each section.                ;
;                                                                       ;
;***********************************************************************;

_z80_init:      push_al_getscr
                %assign i 0
                %rep    256
                mov     word [red_mem_wtwr_table+i],0
                %assign i i+2
                %endrep
                %assign i 0
                %rep    256
                mov     word [red_mem_wtrd_table+i],0
                %assign i i+2
                %endrep
                %assign i 0
                %rep    256
                mov     dword [red_mem_addr_table+i],0
                %assign i i+4
                %endrep
                %assign i 0
                %rep    256
                mov     dword [red_oprd_jump_tble+i],opfetch_call
                %assign i i+4
                %endrep
                %assign i 0
                %rep    256
                mov     dword [red_memrd_jump_tble+i],mrmemrd_call
                %assign i i+4
                %endrep
                %assign i 0
                %rep    256
                mov     dword [red_memwr_jump_tble+i],mrmemwr_call
                %assign i i+4
                %endrep
                call    clear_state                  ; reset the cpu
                mov     [red_local_eax],eax          ; save local copy of
                mov     [red_local_ebx],ebx          ; state for later.
                mov     [red_local_ecx],ecx          ;
                mov     [red_local_edx],edx          ;
                mov     [red_local_esi],esi          ;
                mov     [red_local_edi],edi          ;
                mov     dword [red_retaddr],loop_st  ; set the z80 to enter
                                                     ; at loop_st upon call
                                                     ; to _z80_cycle.
                pop_all
                retn

clear_state:    mov     eax,0x00000ffff              ; clear reg/state to
                mov     ebx,0x00000ffff              ; startup values.
                mov     ecx,0x0ffffffff              ;
                mov     edx,0x00000ffff              ;
                mov     esi,0x0ffffffff              ;
                mov     edi,0x000000000              ;
                mov     word [red_loc_FAx],0x0ffff   ; also clear the stored
                mov     word [red_loc_BCx],0x0ffff   ; version of these.
                mov     word [red_loc_DEx],0x0ffff   ;
                mov     word [red_loc_HLx],0x0ffff   ;
                mov word  [red_z80_clk_bus],0x00000  ; zero the clock bus
                mov dword [red_what_ops],main_opt    ; select opset jumptable
                mov dword [red_what_ret],Z_RET_POINT ; select return point
                mov byte  [red_state_byte],0x00000   ; other misc bits.
                mov word  [red_wait_word],0x00000    ;
                mov byte  [red_z80_wait_bus],0x00000 ; clear other buses
                mov byte  [red_z80_rfsh_bus],0x00000 ;
                mov byte  [red_z80_data_bus],0x00000 ;
                mov word  [red_z80_addr_bus],0x00000 ;
                mov byte  [red_z80_clk_bus],0x00000  ;
           mov word [red_z80_reti_count_bus],0x00000 ;
             mov dword [red_getopbyte],mem_rd_PC_inc ; set opread function.
                retn


;***********************************************************************;
;                                                                       ;
; Call switcher function:                                               ;
;                                                                       ;
;***********************************************************************;


call_switch:    mov     [red_local_eax],eax ; -+ 
                mov     [red_local_ebx],ebx ;  |
                mov     [red_local_ecx],ecx ;  | save the cpu state
                mov     [red_local_edx],edx ;  |
                mov     [red_local_esi],esi ;  |
                mov     [red_local_edi],edi ; -+
                pop     dword [red_retaddr] ; -- pop return address off stack
                pop_all                     ; -- retreive the external state
                retn                        ; -- return to the ext caller
_z80_cycle:     push_al_getscr              ; -\ save the external registers
                                            ; -/ get pointer to scratchpad
                push    dword [red_retaddr] ; -- push return addr onto stack
                mov     edi,[red_local_edi] ; -+ 
                mov     esi,[red_local_esi] ;  |
                mov     edx,[red_local_edx] ;  | retrieve the cpu state
                mov     ecx,[red_local_ecx] ;  |
                mov     ebx,[red_local_ebx] ;  |
                mov     eax,[red_local_eax] ; -+
                retn                        ; -- return to return address


;***********************************************************************;
;                                                                       ;
; Memory mode functions:                                                ;
;                                                                       ;
;***********************************************************************;

_z80_set_mem_write_none: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memwr_jump_tble],mrmemwr_none
                pop_all
                retn
_z80_set_mem_write_direct: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memwr_jump_tble],mrmemwr_mem
                pop_all
                retn
_z80_set_mem_write_indirect: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memwr_jump_tble],mrmemwr_call
                pop_all
                retn

_z80_set_mem_read_none: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memrd_jump_tble],mrmemrd_none
                pop_all
                retn
_z80_set_mem_read_direct: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memrd_jump_tble],mrmemrd_mem
                pop_all
                retn
_z80_set_mem_read_indirect: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memrd_jump_tble],mrmemrd_call
                pop_all
                retn

_z80_set_mem_opread_none: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_oprd_jump_tble],opfetch_none
                pop_all
                retn
_z80_set_mem_opread_direct: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_oprd_jump_tble],opfetch_mem
                pop_all
                retn
_z80_set_mem_opread_indirect: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_oprd_jump_tble],opfetch_call
                pop_all
                retn

_z80_set_mem_write_none_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memwr_jump_tble],mrmemwr_none_naw
                pop_all
                retn
_z80_set_mem_write_direct_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memwr_jump_tble],mrmemwr_mem_naw
                pop_all
                retn
_z80_set_mem_write_indirect_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memwr_jump_tble],mrmemwr_call_naw
                pop_all
                retn

_z80_set_mem_read_none_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memrd_jump_tble],mrmemrd_none_naw
                pop_all
                retn
_z80_set_mem_read_direct_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memrd_jump_tble],mrmemrd_mem_naw
                pop_all
                retn
_z80_set_mem_read_indirect_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_memrd_jump_tble],mrmemrd_call_naw
                pop_all
                retn

_z80_set_mem_opread_none_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_oprd_jump_tble],opfetch_none_naw
                pop_all
                retn
_z80_set_mem_opread_direct_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_oprd_jump_tble],opfetch_mem_naw
                pop_all
                retn
_z80_set_mem_opread_indirect_naw: push_al_getscr
                mov     edi,[red_z80_tab_num_bus]
                and     edi,0x0000000ff
                mov     ax,[red_z80_tab_wr_wait_bus]
                mov     [2*edi+red_mem_wtwr_table],ax
                mov     ax,[red_z80_tab_rd_wait_bus]
                mov     [2*edi+red_mem_wtrd_table],ax
                mov     eax,[red_z80_tab_addr_bus]
                mov     [4*edi+red_mem_addr_table],eax
                mov     dword [4*edi+red_oprd_jump_tble],opfetch_call_naw
                pop_all
                retn


;***********************************************************************;
;                                                                       ;
; Asynchronous operation functions:                                     ;
;                                                                       ;
; _z80_set_reset: set reset bit.                                        ;
; _z80_set_busrq: set bus request bit.                                  ;
; _z80_set_NMI:   set NMI bit.                                          ;
; _z80_set_INT:   set INT bit.                                          ;
; _z80_res_INT:   reset INT bit.                                        ;
; _z80_set_wait:  insert wait states.                                   ;
;                                                                       ;
;***********************************************************************;


_z80_set_reset: push_al_getscr
                or      byte [red_state_byte],0x040
                pop_all
                retn
_z80_set_busrq: push_al_getscr
                or      byte [red_state_byte],0x020
                pop_all
                retn
_z80_set_NMI:   push_al_getscr
                or      byte [red_state_byte],0x010
                pop_all
                retn
_z80_set_INT:   push_al_getscr
                or      byte [red_state_byte],0x008
                pop_all
                retn
_z80_res_INT:   push_al_getscr
                and     byte [red_state_byte],0x0f7
                pop_all
                retn
_z80_set_wait:  push_al_getscr
                push    eax
                mov     eax,[red_z80_wait_bus]
                and     eax,0x0000000ff
                add     word [red_wait_word],ax
                pop     eax
                pop_all
                retn

; macro version

%macro          setwaitz80      0
                push    eax
                mov     eax,[red_z80_wait_bus]
                and     eax,0x0000000ff
                add     word [red_wait_word],ax
                pop     eax
%endmacro



;***********************************************************************;
;                                                                       ;
; External interface functions                                          ;
;                                                                       ;
;***********************************************************************;

; Acknowledge INT externally and get byte to put in edi(l)
; (don't increment PC).  Resets rest of edi
;
; Assumption: - eax is NOT rotated (AF is in ax)

%macro          ack_INT         0
                add     word [red_z80_clk_bus],5
                push_all
                rol     ebx,0x010
                mov     [red_z80_addr_bus],bx
                push    ebp
                call    _z80_ack_INT
                pop     ebp
                pop_all
                mov     edi,[red_z80_data_bus]
                and     edi,0x0000000ff
                setwaitz80
%endmacro


; Memory read functions:
;
; mem_rd_di     - read memory at address di and put result in di(low byte)
; mem_rd_PC_inc - read memory at address PC (bx), put res in di(l) and inc PC
;
; offset_di_for_memrd_HLd: offsets di by [CB_d_off] (offset 2's complement)

%macro          offset_di_for_memrd_HLd 0
                add    word [red_z80_clk_bus],2 ; -- additional cycles
                push    eax                     ; -- save stuff
                mov     al,[red_CB_d_off]       ; -+
                mov     ah,0                    ;  |
                add     al,0                    ;  |
                js      short %%d_doff_down     ;  | offset edi
%%d_doff_up:    add     di,ax                   ;  |
                jmp     short %%d_doff_done     ;  |
%%d_doff_down:  neg     al                      ;  |
                sub     di,ax                   ; -+
%%d_doff_done:  mov     [red_hidden_reg],di     ; -- load hidden word
                pop     eax                     ; -- retrieve stuff
%endmacro

mem_rd_PC_inc:  mov     edi,ebx                 ; -- put PC in edi high word
                add     ebx,0x000010000         ; -- increment PC
                ror     edi,0x010               ; -- put PC into di
                                                ;
mem_rd_di:      add    word [red_z80_clk_bus],3 ; -- memread base is 3 cycles
                push    eax                     ; -- save stuff
                push    edi                     ; -- save addr for later
                ror     edi,0x007               ; -- high_addr now in edi(l)
                and     edi,0x0000001fe         ; -- mask off rest of edi
                jmp [2*edi+red_memrd_jump_tble] ; -- Jump to relevant memory
                                                ;    reading function.
                                                ;
                                                ; == Memory indirect ==
                                                ;
mrmemrd_call:  mov eax,[edi+red_mem_wtrd_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
mrmemrd_call_naw: pop   edi                     ; -\ retrieve address and
                mov     [red_z80_addr_bus],di   ; -/ put on bus
                push_all                        ; -- save state
                push    ebp                     ;
                call    _z80_rd_mem             ; -- call to get result
                pop     ebp                     ;
                pop_all                         ; -- retrieve state
                mov     edi,[red_z80_data_bus]  ; -- put result on edi(l)
                and     edi,0x0000000ff         ; -- mask result
                setwaitz80                      ; -- insert waits if needed
                pop     eax                     ; -- retrieve stuff
                retn                            ;
                                                ;
                                                ; == Memory direct ==
                                                ;
mrmemrd_mem:   mov eax,[edi+red_mem_wtrd_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
mrmemrd_mem_naw: mov edi,[2*edi+red_mem_addr_table] ;-- edi now *start of page
                pop     eax                     ; -\ retrieve address and
                and     eax,0x0000000ff         ; -/ mask
                mov     edi,[edi+eax]           ; -- put result on edi(l)
                and     edi,0x0000000ff         ; -- mask result
                pop     eax                     ; -- retrieve stuff
                retn                            ;
                                                ;
                                                ; == Memory none ==
                                                ;
mrmemrd_none:  mov eax,[edi+red_mem_wtrd_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
mrmemrd_none_naw: pop   edi                     ; -- ignore address
                mov     edi,0                   ; -- load default NOP
                pop     eax                     ; -- retrieve stuff
                retn



; Interupt driven memory read op

INT_rd_PC_inc:  add     word [red_z80_clk_bus],3
                push_all
                ror     ebx,0x010
                mov     [red_z80_addr_bus],bx
                push    ebp
                call    _z80_rd_mem
                pop     ebp
                pop_all
                mov     edi,[red_z80_data_bus]
                and     edi,0x0000000ff
                setwaitz80
                retn


; Memory read functions:
;
; mem_wr_di     - read memory at address di and put result in di(low byte)
;
; offset_di_for_memwr_HLd: offsets di by [CB_d_off] (offset 2's complement)

%macro          offset_mem_for_memwr_HLd 0
                add    word [red_z80_clk_bus],2 ; -- additional cycles
                push    eax                     ; -- save stuff
                mov     al,[red_CB_d_off]       ; -+
                mov     ah,0                    ;  |
                add     al,0                    ;  |
                js      short %%d_doff_down     ;  | offset edi
%%d_doff_up:    add     [red_xx_32_store],ax    ;  |
                jmp     short %%d_doff_done     ;  |
%%d_doff_down:  neg     al                      ;  |
                sub     [red_xx_32_store],ax    ; -+
%%d_doff_done:  pop     eax                     ; -- retrieve stuff
%endmacro


mem_wr_di:      push    eax                     ; -- save stuff
                push    edi                     ; -- save what we're writing
                add    word [red_z80_clk_bus],3 ; -- memwrite base 3 cycles
                mov     edi,[red_xx_32_store]   ; -\ high_addr now in edi(l)
                ror     edi,0x007               ; -/
                and     edi,0x0000001fe         ; -- mask off rest of edi
                jmp [2*edi+red_memwr_jump_tble] ;
                                                ;
                                                ; == Memory indirect ==
                                                ;
mrmemwr_call:  mov eax,[edi+red_mem_wtwr_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
mrmemwr_call_naw: mov   edi,[red_xx_32_store]   ; -\ addr now on bus
                mov     [red_z80_addr_bus],di   ; -/
                mov     edi,eax                 ; -- save eax
                pop     eax                     ; -\ data now on bus
                mov     [red_z80_data_bus],al   ; -/
                mov     eax,edi                 ; -- put eax back
                push_all                        ; -- save state
                push    ebp                     ;
                call    _z80_wr_mem             ; -- call to get result
                pop     ebp                     ;
                pop_all                         ; -- retrieve state
                setwaitz80                      ; -- insert waits if needed
                pop     eax                     ; -- retrieve stuff
                retn                            ;
                                                ;
                                                ; == Memory direct ==
                                                ;
mrmemwr_mem:   mov eax,[edi+red_mem_wtwr_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
mrmemwr_mem_naw: mov edi,[2*edi+red_mem_addr_table] ;-- edi now *start of page
                mov     eax,[red_xx_32_store]   ; -\ low_addr now in al
        and dword [red_xx_32_store],0x0000000ff ; -/
                add     edi,[red_xx_32_store]   ; -- edi now mem pointer
                pop     eax                     ; -\ write to memory
                mov     [edi],al                ; -/
                pop     eax                     ; -- retrieve stuff
                retn                            ;
                                                ;
                                                ; == Memory none ==
                                                ;
mrmemwr_none:  mov eax,[edi+red_mem_wtwr_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
mrmemwr_none_naw: pop   eax                     ; -- ignore write
                pop     eax                     ; -- retrieve stuff
                retn                            ;





; io read edi(l) byte from BC
; io read edi(l) byte from di
;
; Assumption: - eax is NOT rotated (AF is in ax)

%macro          io_rd_xx        1
                add    word [red_z80_clk_bus],4 ; -- ioread base is 4 cycles
                push_all                        ; -- save state
                mov     [red_z80_addr_bus],%1   ; -- addr now on bus
                push    ebp                     ;
                call    _z80_rd_io              ; -- call to get result
                pop     ebp                     ;
                pop_all                         ; -- retrieve state
                mov     edi,[red_z80_data_bus]  ; -- get result
                and     edi,0x0000000ff         ; -- mask result
                setwaitz80                      ; -- insert waits if needed
%endmacro

io_rd_BC:       io_rd_xx bx
                retn
io_rd_di:       push    ebx
                mov     ebx,edi
                io_rd_xx bx
                pop     ebx
                retn

; io write to BC with edi(l) byte
; io write to (u)di with edi(l) byte
;
; Assumption: - eax is NOT rotated (AF is in ax)

%macro          io_wr_xx        1
                push    eax                     ; -- save stuff
                add    word [red_z80_clk_bus],4 ; -- iowrite base 4 cycles
                push_all                        ; -- save state
                mov     [red_z80_addr_bus],%1   ; -- addr now on bus
                mov     eax,edi                 ; -\ data now on bus
                mov     [red_z80_data_bus],al   ; -/
                push    ebp                     ;
                call    _z80_wr_io              ; -- call to get result
                pop     ebp                     ;
                pop_all                         ; -- retrieve state
                setwaitz80                      ; -- insert waits if needed
                pop     eax                     ; -- retreive stuff
%endmacro

io_wr_BC:       io_wr_xx bx
                retn
io_wr_di:       push    ebx
                mov     ebx,edi
                ror     ebx,0x010               ; Rotation to get UPPER part
                io_wr_xx bx
                pop     ebx
                retn






;***********************************************************************;
;                                                                       ;
; Main emulator loop starts here                                        ;
;                                                                       ;
;***********************************************************************;

; T-state default macro for all non-fast processes.

%macro          default_Ts      0
                mov edi,[red_wait_word] ; Get external T-state count.
               add [red_z80_clk_bus],di ; Add to internal T-state count.
             mov word [red_wait_word],0 ; Clear external T-state count.
                call    call_switch     ; "return" to caller, temporarily.
                                        ; NB. This operation is not
                                        ;     quite atomic, so T-states
                                        ;     may occasionally be lost.
%endmacro

; R update macro - increment and decrement low 7 bits.

%macro          increm_R        0       ; 
                ror     edx,0x00f       ; Switch in IR.
                                        ; Put bit 8 out of R into bit 1 of I,
                                        ; and bit 8 or I into bit 1 of R.
                add     dl,0x002        ; Add 2 to dl.  This will not change
                                        ; bit 1, or dh.  Hence the low 7 bits
                                        ; of R are incremented, but bit 8
                                        ; of R and all of I are unchanged.
                                        ; Put IR alignment back to normal.
                rol     edx,0x00f       ; Switch out IR.
                                        ; 
                                        ; NB: DDCB and FDCB only inc R
                                        ;     by one, so need to dec R
                                        ;     in CB prefix call if DD/FD
                                        ;     is present.
%endmacro

%macro          decrem_R        0       ; 
                ror     edx,0x00f       ; Switch in IR.
                                        ; Put bit 8 out of R into bit 1 of I,
                                        ; and bit 8 or I into bit 1 of R.
                sub     dl,0x002        ; Add 2 to dl.  This will not change
                                        ; bit 1, or dh.  Hence the low 7 bits
                                        ; of R are incremented, but bit 8
                                        ; of R and all of I are unchanged.
                                        ; Put IR alignment back to normal.
                rol     edx,0x00f       ; Switch out IR.
%endmacro


; Top of emulation loop.

loop_st:        ror     eax,0x010       ; Put state bytes in ax.
                and     eax,0x0ffffff03 ; Remove previous signals.
                or  al,[red_state_byte] ; Get current signals.
                mov     edi,eax         ; Put state in edi
                and     edi,0x0000003ff ; Mask out all but relevant state.
                jmp   [4*edi+state_jmp] ; Jump to correct operation for
                                        ; the current state.

; Signal illegal operation, hit the reset.

loop_error:     rol     eax,0x010       ; Put FA/state back in correct form.
                push_all                ; save state
                push    ebp             ;
                call    _z80_sig_error  ; send error signal
                pop     ebp             ;
                pop_all                 ; retrieve state
                call    clear_state     ; Clear the CPU state (clean start).
                jmp     near loop_st    ; Start CPU again.


; Reset process

loop_res: and byte [red_state_byte],0x0bf ; Clear reset signal.
                rol     eax,0x010       ; Put FA/state back in correct form.
                push_all                ; save state
                push    ebp             ;
                call    _z80_ack_reset  ; acknowledge reset
                pop     ebp             ;
                pop_all                 ; retrieve state
                call    clear_state     ; Clear the CPU state.
                jmp     near loop_st    ; Start CPU again.

; Bus request process

loop_brq:       default_Ts
        and byte [red_state_byte],0x0df ; Clear busrq signal.
                rol     eax,0x010       ; Put FA/state back in correct form.
           add word [red_z80_clk_bus],1 ; -+
                push_all                ;  |
                push    ebp             ;  |
                call    _z80_ack_busrq  ;  | Call fn to acknowledge busrq.
                pop     ebp             ;  |
                pop_all                 ;  |
                setwaitz80              ; -+
                jmp     near loop_st    ; Go back to start of loop.

; NMI process

loop_nmi:
loop_nmi_h:     default_Ts
        and byte [red_state_byte],0x0ef ; Clear NMI signal.
                and     eax,0x0fffffdfd ; Clear IFF1 and HALT bits.
                rol     eax,0x010       ; Put FA/state back in correct form.
           add word [red_z80_clk_bus],5 ; -+
                push_all                ;  |
                rol     ebx,0x010       ;  |
                rol     edx,0x010       ;  | Call fn to acknowledge NMI.
              mov [red_z80_addr_bus],bx ;  | 
              mov [red_z80_rfsh_bus],dl ;  |
                push    ebp             ;  |
                call    _z80_ack_NMI    ;  |
                pop     ebp             ;  |
                pop_all                 ; -+
                ror     ecx,0x010       ; Switch in SP.
                ror     ebx,0x010       ; Switch in PC.
                dec     cx              ; Decrement SP.
                xchg    edi,eax         ; -+
                mov     al,bh           ;  | ld (SP),PCh.
                xchg    edi,eax         ;  |
                rol     ecx,0x010       ;  |
                rol     ebx,0x010       ;  |
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di       ;  |
                ror     ecx,0x010       ;  |
                ror     ebx,0x010       ; -+
                dec     cx              ; Decrement SP.
                xchg    edi,eax         ; -+
                mov     al,bl           ;  | ld (SP),PCl.
                xchg    edi,eax         ;  |
                rol     ecx,0x010       ;  |
                rol     ebx,0x010       ;  |
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di       ;  |
                ror     ecx,0x010       ;  |
                ror     ebx,0x010       ; -+
                mov     bx,0x00066      ; ld PC,0066h.
                rol     ecx,0x010       ; Switch out SP.
                rol     ebx,0x010       ; Switch out PC.
                jmp     near loop_st    ; Go back to start of loop.

; INT process

loop_int:
loop_int_h:     default_Ts
                and     eax,0x0fffff9fd ; Clear IFF1, IFF2 and HALT bits.
                                        ;
                                        ; The INT routine is as follows: 
                test    eax,0x000001000 ;
                jnz     short INTIM12   ; 
                                        ;                    -+
INTIM0:         or      eax,0x000000001 ; -- Set INTOP bit    |
                mov   edi,INT_rd_PC_inc ; -\ switch to get    | IM 0
                mov [red_getopbyte],edi ; -/ ops from INT     | (00,01)
                jmp    near loop_intp_q ; -- ret int dispatch |
                                        ;                    -+
INTIM12:        test    eax,0x000000800 ;
                jnz     near INTIM2     ;
                                        ;                    -+
INTIM1:         rol     eax,0x010       ; -- get byte off     |
                ack_INT                 ; -- int bus, ignore  |
                ror     ecx,0x010       ; -- switch in SP     |
                ror     ebx,0x010       ; -- switch in PC     |
                dec     cx              ; -- Decrement SP     |
                xchg    edi,eax         ; -+                  |
                mov     al,bh           ;  | ld (SP),PCh      |
                xchg    edi,eax         ;  |                  | IM 1
                rol     ebx,0x010       ;  |                  |
                rol     ecx,0x010       ;  |                  |
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di       ;  |                  |
                ror     ecx,0x010       ;  |                  |
                ror     ebx,0x010       ; -+                  |
                dec     cx              ; -- Decrement SP     | (10)
                xchg    edi,eax         ; -+                  |
                mov     al,bl           ;  | ld (SP),PCl      |
                xchg    edi,eax         ;  |                  |
                rol     ebx,0x010       ;  |                  |
                rol     ecx,0x010       ;  |                  |
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di       ;  |                  |
                ror     ecx,0x010       ;  |                  |
                ror     ebx,0x010       ; -+                  |
                mov     bx,0x00038      ; -- ld PC,0038h      |
                rol     ecx,0x010       ; -- switch out SP    |
                rol     ebx,0x010       ; -- switch out PC    |
                jmp     near loop_st    ; -- return to start  |
                                        ;                    -+
                                        ;
                                        ;                    -+
INTIM2:         rol     eax,0x010       ; -- get byte EDI(l)  |
                ack_INT                 ; -- off int bus      |
                and     edi,0x0000000ff ; -- mask byte off    |
                push    edi             ; -- save edi         |
                ror     ecx,0x010       ; -- switch in SP     |
                ror     ebx,0x010       ; -- switch in PC     |
                ror     edx,0x010       ; -- switch in IR     |
                dec     cx              ; -- Decrement SP     |
                xchg    edi,eax         ; -+                  |
                mov     al,bh           ;  | ld (SP),PCh      |
                xchg    edi,eax         ;  |                  |     
                rol     ebx,0x010       ;  |                  |
                rol     ecx,0x010       ;  |                  |
                rol     edx,0x010       ;  |                  |
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di       ;  |                  |
                ror     edx,0x010       ;  |                  |
                ror     ecx,0x010       ;  |                  |
                ror     ebx,0x010       ; -+                  |
                dec     cx              ; -- Decrement SP     |
                xchg    edi,eax         ; -+                  |
                mov     al,bl           ;  | ld (SP),PCl      | IM 2
                xchg    edi,eax         ;  |                  | (11)
                rol     ebx,0x010       ;  |                  |
                rol     ecx,0x010       ;  |                  |
                rol     edx,0x010       ;  |                  |
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di       ;  |                  |
                ror     edx,0x010       ;  |                  |
                ror     ecx,0x010       ;  |                  |
                ror     ebx,0x010       ; -+                  |
                pop     edi             ; -- retrieve data    |
                push    ecx             ; -- save SP          |
                mov     ecx,edi         ; -- get byte of bus  |
                mov     ch,dh           ; -- add I*256 -> addr|
                rol     ebx,0x010       ; -+                  |
                rol     edx,0x010       ;  |                  |
                mov     edi,ecx         ;  |                  |
                rol     ecx,0x010       ;  |                  |
                call    mem_rd_di       ;  | rd mem to EDI(l) |
                ror     ebx,0x010       ;  |                  |
                ror     ecx,0x010       ;  |                  |
                ror     edx,0x010       ; -+                  |
                xchg    eax,edi         ; -+                  |
                mov     bl,al           ;  | put into PCl     |
                xchg    eax,edi         ; -+                  |
                inc     cx              ; -+ onto next addr   |
                rol     ebx,0x010       ;  |                  |
                rol     edx,0x010       ;  |                  |
                mov     edi,ecx         ;  |                  |
                rol     ecx,0x010       ;  |                  |
                call    mem_rd_di       ;  | rd mem to EDI(l) |
                rol     ebx,0x010       ;  |                  |
                rol     ecx,0x010       ;  |                  |
                rol     edx,0x010       ; -+                  |
                xchg    eax,edi         ; -+                  |
                mov     bh,al           ;  | put into PCh     |
                xchg    eax,edi         ; -+                  |
                pop     ecx             ; -- restore SP       |
                rol     edx,0x010       ; -- switch out IR    |
                rol     ecx,0x010       ; -- switch out SP    |
                rol     ebx,0x010       ; -- switch out PC    |
                jmp     near loop_st    ; -- return to start  |
                                        ;                    -+



; Midway INT opfetch process

loop_intp:      default_Ts
loop_intp_q:    increm_R
                rol     eax,0x010       ; Put FA/state back in correct form.
                ack_INT                 ; Get opcode with ack_INT call.
                jmp     near op_disp    ; Goto opcode dispatcher.

; HALT opfetch process

loop_halt:      mov     dword [red_op_mask],0x000000000 ; force NOP opcode
                mov     dword [red_PC_incr],0x000000000 ; no PC increment
                jmp     near opload


; Normal opfetch process

loop_norm:      mov     dword [red_op_mask],0x0000000ff ; no opcode mask
                mov     dword [red_PC_incr],0x000010000 ; PC increment

opload:         default_Ts              ; T states
                increm_R                ; Increment and mask refresh reg
                rol     eax,0x010       ; Put FA/state back in correct form.
                push    eax             ; Save stuff
                                        ; Now fetch opcode:
                                        ;
                                        ;;;;;;;;;
                                                ;
                add    word [red_z80_clk_bus],4 ; -- opfetch base is 4 cycles
                mov     edi,ebx                 ; -- put PC in upper 16 edi
                rol     edi,0x009               ; -- put PCh in edi(l)
                and     edi,0x0000001fe         ; -- mask off rest of edi
                jmp  [2*edi+red_oprd_jump_tble] ; -- Jump to relevant opcode
                                                ;    reading function.
                                                ;
                                                ; === No memory fetch ===
                                                ;
opfetch_none:  mov eax,[edi+red_mem_wtrd_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
opfetch_none_naw: mov   edi,0                   ; Load default NOP.
                add     ebx,[red_PC_incr]       ; Modify PC appropriately.
                pop     eax                     ; Retrieve stuff
                jmp     near op_disp            ; Goto opcode dispatcher.
                                                ;
                                                ; === Direct access fetch ===
                                                ;
opfetch_mem:   mov eax,[edi+red_mem_wtrd_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
opfetch_mem_naw: mov edi,[2*edi+red_mem_addr_table] ;-- edi now *start of page
                mov     eax,ebx                 ; -+
                ror     eax,0x010               ;  | PCl now in al
                and     eax,0x0000000ff         ; -+
                mov     edi,[edi+eax]           ; -- load mem into edi(l)
                and     edi,[red_op_mask]       ; Mask opcode.
                add     ebx,[red_PC_incr]       ; Modify PC appropriately.
                pop     eax                     ; Retrieve stuff
                jmp     near op_disp            ; Goto opcode dispatcher.
                                                ;
                                                ; == Indirect access fetch ==
                                                ;
opfetch_call:  mov eax,[edi+red_mem_wtrd_table] ; -\ Add wait states from
                add     [red_z80_clk_bus],ax    ; -/ table to counter.
opfetch_call_naw: pop   eax                     ; Retrieve stuff
                push_all                        ; -- save state
                rol     ebx,0x010               ; -- switch in PC
                rol     edx,0x010               ; -- switch in IR
                mov     [red_z80_addr_bus],bx   ; -- put PC on addr bus
                mov     [red_z80_rfsh_bus],dl   ; -- put R on refresh bus
                push    ebp                     ;
                call    _z80_opfetch            ; -- call to get opcode
                pop     ebp                     ;
                pop_all                         ; -- retrieve state
                mov     edi,[red_z80_data_bus]  ; -- put opcode on edi(l)
                setwaitz80                      ; -- insert waits if needed
                and     edi,[red_op_mask]       ; Mask opcode.
                add     ebx,[red_PC_incr]       ; Modify PC appropriately.
                jmp     near op_disp            ; Goto opcode dispatcher.
                                                ;
                                        ;;;;;;;;;
                                        ;








; Opcode dispatcher

op_disp:        and     edi,0x0000000ff ; Mask off opcode
                rcl     edi,2           ; Multiply opcode by 4 (each opcode
                                        ; in the call table occupies 4 bytes,
                                        ; so the offset for an op is the 
                                        ; opcode*4).
                add  edi,[red_what_ops] ; Offset to start of jump table.
                jmp     [edi]           ; Jump to relevant op.


;***********************************************************************;
;                                                                       ;
; Macro to get offset and save in [CB_d_off].                           ;
;                                                                       ;
;***********************************************************************;

%macro          get_d_off       0
                call    [red_getopbyte] ; -- get d into edi(l)
                xchg    eax,edi         ; -+
                mov   [red_CB_d_off],al ;  | Save d for later
                xchg    eax,edi         ; -+
%endmacro


;***********************************************************************;
;                                                                       ;
; Return points:                                                        ;
;                                                                       ;
; - fix jump and return table addresses.                                ;
; - fix getopbyte to get ops from memory.                               ;
; - fix DI and INTOP bytes as appropriate.                              ;
; - switch IX,IY and HL as is necessary.                                ;
;                                                                       ;
;***********************************************************************;

Z_RET_POINT:    and     eax,0x01efeffff ; -- clear DI and INTOP
Z_RET_POINTx:                           ; -- set pref_mode = 000
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                jmp     near loop_st
ZDD_RET_POINT:  and     eax,0x01efeffff ; -- clear DI and INTOP
ZDD_RET_POINTx:
   mov [red_what_ops],dword main_opt    ; -- reset jump table
   mov [red_what_ret],dword Z_RET_POINT ; -- reset return point
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                xchg    dx,si           ; -- switch IX and HL
                jmp     near loop_st
ZFD_RET_POINT:  and     eax,0x01efeffff ; -- clear DI and INTOP
ZFD_RET_POINTx:
   mov [red_what_ops],dword main_opt    ; -- reset jump table
   mov [red_what_ret],dword Z_RET_POINT ; -- reset return point
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                ror     esi,0x010       ; -+
                xchg    dx,si           ;  | switch IY and HL
                rol     esi,0x010       ; -+
                jmp     near loop_st

ZCB_RET_POINT:  and     eax,0x01efeffff ; -- clear DI and INTOP
   mov [red_what_ops],dword main_opt    ; -- reset jump table
   mov [red_what_ret],dword Z_RET_POINT ; -- reset return point
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                jmp     near loop_st
ZDDCB_RET_POINT: and    eax,0x01efeffff ; -- clear DI and INTOP
   mov [red_what_ops],dword main_opt    ; -- reset jump table
   mov [red_what_ret],dword Z_RET_POINT ; -- reset return point
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                xchg    dx,si           ; -- switch IX and HL
                jmp     near loop_st
ZFDCB_RET_POINT: and    eax,0x01efeffff ; -- clear DI and INTOP
   mov [red_what_ops],dword main_opt    ; -- reset jump table
   mov [red_what_ret],dword Z_RET_POINT ; -- reset return point
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                ror     esi,0x010       ; -+
                xchg    dx,si           ;  | switch IY and HL
                rol     esi,0x010       ; -+
                jmp     near loop_st

ZED_RET_POINT:  and     eax,0x01efeffff ; -- clear DI and INTOP
   mov [red_what_ops],dword main_opt    ; -- reset jump table
   mov [red_what_ret],dword Z_RET_POINT ; -- reset return point
                mov   edi,mem_rd_PC_inc ; -\ switch to get
                mov [red_getopbyte],edi ; -/ ops from memory
                jmp     near loop_st

ZED_RETIN_RETP: mov     edi,eax         ; -- duplicate
                and     edi,0x01dffffff ; -- mask out IFF1
                and     eax,0x004000000 ; -- mask in IFF2
                ror     eax,0x001       ; -- rotate to IFF1
                or      eax,edi         ; -- overwrite
                jmp short ZED_RET_POINT ; -- rest is standard




;***********************************************************************;
;                                                                       ;
; Processor control opcodes                                             ;
;                                                                       ;
; - fix jump and return table addresses.                                ;
; - fix getopbyte to get ops from memory.                               ;
; - fix DI and INTOP bytes as appropriate.                              ;
;                                                                       ;
;***********************************************************************;


ZFD_DD_PREF     ror     esi,0x010       ; -+
                xchg    dx,si           ;  | switch IY and HL
                rol     esi,0x010       ; -+
Z_DD_PREF       or      eax,0x001000000 ; -- set DI bit
 mov [red_what_ops],dword main_opt_DD   ; -- use DD jump table
 mov [red_what_ret],dword ZDD_RET_POINT ; -- use DD ret point
                xchg    dx,si           ; -- switch IX and HL
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x020000000 ; -- set pref mode bits 001
ZDD_DD_PREF     jmp     near loop_st


ZDD_FD_PREF     xchg    dx,si           ; -- switch IX and HL
Z_FD_PREF       or      eax,0x001000000 ; -- set DI bit
 mov [red_what_ops],dword main_opt_FD   ; -- use FD jump table
 mov [red_what_ret],dword ZFD_RET_POINT ; -- use FD ret point
                ror     esi,0x010       ; -+
                xchg    dx,si           ;  | switch IY and HL
                rol     esi,0x010       ; -+
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x040000000 ; -- set pref mode bits 010
ZFD_FD_PREF     jmp     near loop_st

Z_ED_PREF       or      eax,0x001000000 ; -- set DI bit
 mov [red_what_ops],dword ED_opt        ; -- use ED jump table
 mov [red_what_ret],dword ZED_RET_POINT ; -- use ED ret point
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x0e0000000 ; -- set pref mode bits 111
                jmp     near loop_st
ZDD_ED_PREF     or      eax,0x001000000 ; -- set DI bit
 mov [red_what_ops],dword ED_opt        ; -- use ED jump table
 mov [red_what_ret],dword ZED_RET_POINT ; -- use ED ret point
                xchg    dx,si           ; -- switch IX and HL
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x0e0000000 ; -- set pref mode bits 111
                jmp     near loop_st
ZFD_ED_PREF     or      eax,0x001000000 ; -- set DI bit
 mov [red_what_ops],dword ED_opt        ; -- use ED jump table
 mov [red_what_ret],dword ZED_RET_POINT ; -- use ED ret point
                ror     esi,0x010       ; -+
                xchg    dx,si           ;  | switch IY and HL
                rol     esi,0x010       ; -+
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x0e0000000 ; -- set pref mode bits 111
                jmp     near loop_st

Z_CB_PREF       or      eax,0x001000000 ; -- set DI bit
 mov [red_what_ops],dword CB_opt        ; -- use CB jump table
 mov [red_what_ret],dword ZCB_RET_POINT ; -- use CB ret point
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x080000000 ; -- set pref mode bits 100
                jmp     near loop_st
ZDD_CB_PREF:    or      eax,0x001000000 ; -- set DI bit
mov [red_what_ops],dword CB_opt_DD      ; -- use DDCB jump table
mov [red_what_ret],dword ZDDCB_RET_POINT ;-- use DDCB ret point
                get_d_off               ; -- get d offset and save
                decrem_R                ; -- decrement R
                or      eax,0x001000000 ; -- set DI bit *again*
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x0a0000000 ; -- set pref mode bits 100
                jmp     near loop_st
ZFD_CB_PREF:    or      eax,0x001000000 ; -- set DI bit
mov [red_what_ops],dword CB_opt_FD      ; -- use FDCB jump table
mov [red_what_ret],dword ZFDCB_RET_POINT ;-- use FDCB ret point
                get_d_off               ; -- get d offset and save
                decrem_R                ; -- decrement R
                or      eax,0x001000000 ; -- set DI bit *again*
                and     eax,0x01fffffff ; -- mask off pref mode bits
                or      eax,0x0c0000000 ; -- set pref mode bits 100
                jmp     near loop_st

Z_HALT:         or      eax,0x000020000 ; -+ - set HALTED bit   HALT
                push_all                ;  |
                push    ebp             ;  |
                call    _z80_ack_halt   ;  | - acknowledge HALT
                pop     ebp             ;  |
                pop_all                 ;  |
                jmp    near Z_RET_POINT ; -+ - rest is standard
ZDD_HALT:       or      eax,0x000020000 ; -+ - set HALTED bit   HALT
                push_all                ;  |
                push    ebp             ;  |
                call    _z80_ack_halt   ;  | - acknowledge HALT
                pop     ebp             ;  |
                pop_all                 ;  |
                jmp  near ZDD_RET_POINT ; -+ - rest is standard
ZFD_HALT:       or      eax,0x000020000 ; -+ - set HALTED bit   HALT
                push_all                ;  |
                push    ebp             ;  |
                call    _z80_ack_halt   ;  | - acknowledge HALT
                pop     ebp             ;  |
                pop_all                 ;  |
                jmp  near ZFD_RET_POINT ; -+ - rest is standard

Z_DI            and     eax,0x0f9ffffff ; -+ - clear IFF1/2     DI
                or      eax,0x001000000 ;  | - set DI bit
                and     eax,0x01ffeffff ;  | - clear INTOP bit
                jmp   near Z_RET_POINTx ; -+ - rest near normal
ZDD_DI          and     eax,0x0f9ffffff ; -+ - clear IFF1/2     DI
                or      eax,0x001000000 ; -+ - set DI bit
                and     eax,0x01ffeffff ;  | - clear INTOP bit
                jmp near ZDD_RET_POINTx ; -+ - rest near normal
ZFD_DI          and     eax,0x0f9ffffff ; -+ - clear IFF1/2     DI
                or      eax,0x001000000 ; -+ - set DI bit
                and     eax,0x01ffeffff ;  | - clear INTOP bit
                jmp near ZFD_RET_POINTx ; -+ - rest near normal

Z_EI            or      eax,0x006000000 ; -+ - set IFF1/2       EI
                or      eax,0x001000000 ;  | - set DI bit
                and     eax,0x01ffeffff ;  | - clear INTOP bit
                jmp   near Z_RET_POINTx ; -+ - rest near normal
ZDD_EI          or      eax,0x006000000 ; -+ - set IFF1/2       EI
                or      eax,0x001000000 ; -+ - set DI bit
                and     eax,0x01ffeffff ;  | - clear INTOP bit
                jmp near ZDD_RET_POINTx ; -+ - rest near normal
ZFD_EI          or      eax,0x006000000 ; -+ - set IFF1/2       EI
                or      eax,0x001000000 ; -+ - set DI bit
                and     eax,0x01ffeffff ;  | - clear INTOP bit
                jmp near ZFD_RET_POINTx ; -+ - rest near normal

ZED_IM_0:       and     eax,0x0e7000000 ; -- IM = 00            IM 0
                or      eax,0x000000000 ; -- IM = 00
                jmp  near ZED_RET_POINT ; -- rest is standard
ZED_IM_1:       and     eax,0x0e7000000 ; -- IM = 00            IM 1
                or      eax,0x010000000 ; -- IM = 10
                jmp  near ZED_RET_POINT ; -- rest is standard
ZED_IM_2:       and     eax,0x0e7000000 ; -- IM = 00            IM 2
                or      eax,0x018000000 ; -- IM = 11
                jmp  near ZED_RET_POINT ; -- rest is standard







;***********************************************************************;
;***********************************************************************;
;                                                                       ;
;***********************************************************************;
;                                                                       ;
; Standard opcodes start here                                           ;
;                                                                       ;
;***********************************************************************;
;                                                                       ;
;***********************************************************************;
;***********************************************************************;

; When dealing with mem/io, the following indirected calls should be
; used rather than direct calls:
;
; call [getopbyte] = get additional opcode bytes.  Will also increment
;                    PC if relevant (ie. unless midway through INTfetch
;                    cycle).  Opcode read is placed in edi(l).
; call [memrd_xx]  = read edi(l) from mem xx, where xx = BC,DE,HL,SP,di
; call [memwr_xx]  = write edi(l) to mem xx, where xx = BC,DE,HL,SP,di
; call [iord_xx]   = read edi(l) from io xx, where xx = BC,di
; call [iowr_xx]   = write edi(l) to io xx, where xx = BC,di
; jmp [what_ret]   = exit from function in standard way
;
; NOTES - mem_rd_di, io_rd_di both use LOWER 16 bits of edi
;       - mem_wr_di, io_wr_di both use UPPER 16 bits of edi
;       - in either case, the address will be overwritten.
;       - [CB_d_off] stores d for xDCB opcodes


; Nothing opcodes

Z_NOP           jmp     [red_what_ret]  ;                       NOP
ZED_NOP:        jmp     [red_what_ret]  ;                       EDNOP


; 8-bit Load opcodes

Z_LD_Bcn        call    [red_getopbyte] ;                       LD B,n
                xchg    eax,edi
                mov     bh,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_Ccn        call    [red_getopbyte] ;                       LD C,n
                xchg    eax,edi
                mov     bl,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_Dcn        call    [red_getopbyte] ;                       LD D,n
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_Ecn        call    [red_getopbyte] ;                       LD E,n
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_Hcn        call    [red_getopbyte] ;                       LD H,n
                xchg    eax,edi
                mov     dh,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_Lcn        call    [red_getopbyte] ;                       LD L,n
                xchg    eax,edi
                mov     dl,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_Acn        call    [red_getopbyte] ;                       LD A,n
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                jmp     [red_what_ret]
Z_LD_BcB        jmp     [red_what_ret]  ;                       LD B,B
Z_LD_BcC        mov     bh,bl           ;                       LD B,C
                jmp     [red_what_ret]
Z_LD_BcD        mov     bh,ch           ;                       LD B,D
                jmp     [red_what_ret]
Z_LD_BcE        mov     bh,cl           ;                       LD B,E
                jmp     [red_what_ret]
Z_LD_BcH        mov     bh,dh           ;                       LD B,H
                jmp     [red_what_ret]
Z_LD_BcL        mov     bh,dl           ;                       LD B,L
                jmp     [red_what_ret]
Z_LD_BcA        mov     bh,al           ;                       LD B,A
                jmp     [red_what_ret]
Z_LD_CcB        mov     bl,bh           ;                       LD C,B
                jmp     [red_what_ret]
Z_LD_CcC        jmp     [red_what_ret]      ;                       LD C,C
Z_LD_CcD        mov     bl,ch           ;                       LD C,D
                jmp     [red_what_ret]
Z_LD_CcE        mov     bl,cl           ;                       LD C,E
                jmp     [red_what_ret]
Z_LD_CcH        mov     bl,dh           ;                       LD C,H
                jmp     [red_what_ret]
Z_LD_CcL        mov     bl,dl           ;                       LD C,L
                jmp     [red_what_ret]
Z_LD_CcA        mov     bl,al           ;                       LD C,A
                jmp     [red_what_ret]
Z_LD_DcB        mov     ch,bh           ;                       LD D,B
                jmp     [red_what_ret]
Z_LD_DcC        mov     ch,bl           ;                       LD D,C
                jmp     [red_what_ret]
Z_LD_DcD        jmp     [red_what_ret]      ;                       LD D,D
Z_LD_DcE        mov     ch,cl           ;                       LD D,E
                jmp     [red_what_ret]
Z_LD_DcH        mov     ch,dh           ;                       LD D,H
                jmp     [red_what_ret]
Z_LD_DcL        mov     ch,dl           ;                       LD D,L
                jmp     [red_what_ret]
Z_LD_DcA        mov     ch,al           ;                       LD D,A
                jmp     [red_what_ret]
Z_LD_EcB        mov     cl,bh           ;                       LD E,B
                jmp     [red_what_ret]
Z_LD_EcC        mov     cl,bl           ;                       LD E,C
                jmp     [red_what_ret]
Z_LD_EcD        mov     cl,ch           ;                       LD E,D
                jmp     [red_what_ret]
Z_LD_EcE        jmp     [red_what_ret]      ;                       LD E,E
Z_LD_EcH        mov     cl,dh           ;                       LD E,H
                jmp     [red_what_ret]
Z_LD_EcL        mov     cl,dl           ;                       LD E,L
                jmp     [red_what_ret]
Z_LD_EcA        mov     cl,al           ;                       LD E,A
                jmp     [red_what_ret]
Z_LD_HcB        mov     dh,bh           ;                       LD H,B
                jmp     [red_what_ret]
Z_LD_HcC        mov     dh,bl           ;                       LD H,C
                jmp     [red_what_ret]
Z_LD_HcD        mov     dh,ch           ;                       LD H,D
                jmp     [red_what_ret]
Z_LD_HcE        mov     dh,cl           ;                       LD H,E
                jmp     [red_what_ret]
Z_LD_HcH        jmp     [red_what_ret]      ;                       LD H,H
Z_LD_HcL        mov     dh,dl           ;                       LD H,L
                jmp     [red_what_ret]
Z_LD_HcA        mov     dh,al           ;                       LD H,A
                jmp     [red_what_ret]
Z_LD_LcB        mov     dl,bh           ;                       LD L,B
                jmp     [red_what_ret]
Z_LD_LcC        mov     dl,bl           ;                       LD L,C
                jmp     [red_what_ret]
Z_LD_LcD        mov     dl,ch           ;                       LD L,D
                jmp     [red_what_ret]
Z_LD_LcE        mov     dl,cl           ;                       LD L,E
                jmp     [red_what_ret]
Z_LD_LcH        mov     dl,dh           ;                       LD L,H
                jmp     [red_what_ret]
Z_LD_LcL        jmp     [red_what_ret]      ;                       LD L,L
Z_LD_LcA        mov     dl,al           ;                       LD L,A
                jmp     [red_what_ret]
Z_LD_AcB        mov     al,bh           ;                       LD A,B
                jmp     [red_what_ret]
Z_LD_AcC        mov     al,bl           ;                       LD A,C
                jmp     [red_what_ret]
Z_LD_AcD        mov     al,ch           ;                       LD A,D
                jmp     [red_what_ret]
Z_LD_AcE        mov     al,cl           ;                       LD A,E
                jmp     [red_what_ret]
Z_LD_AcH        mov     al,dh           ;                       LD A,H
                jmp     [red_what_ret]
Z_LD_AcL        mov     al,dl           ;                       LD A,L
                jmp     [red_what_ret]
Z_LD_AcA        jmp     [red_what_ret]      ;                       LD A,A

Z_LD_bBCbcA     xchg    ebx,edi         ;                       LD (BC),A
                mov     bl,al
                xchg    ebx,edi
                mov     [red_xx_32_store],ebx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bDEbcA     xchg    ebx,edi         ;                       LD (DE),A
                mov     bl,al
                xchg    ebx,edi
                mov     [red_xx_32_store],ecx
                call    mem_wr_di
                jmp     [red_what_ret]

Z_LD_AcbBCb     mov     edi,ebx         ;                       LD A,(BC)
                call    mem_rd_di
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                jmp     [red_what_ret]
Z_LD_AcbDEb     mov     edi,ecx         ;                       LD A,(DE)
                call    mem_rd_di
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                jmp     [red_what_ret]



Z_LD_bnnbcA     call    [red_getopbyte] ;                       LD (nn),A
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                rol     edi,0x010
                mov     di,ax
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]

Z_LD_Acbnnb     call    [red_getopbyte] ;                       LD A,(nn)
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                call    mem_rd_di
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                jmp     [red_what_ret]

Z_LD_bHLbcn     call    [red_getopbyte] ;                       LD (HL),n
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]

ZxD_LD_bHLbcn   get_d_off               ;                       LD (Ix+d),n
                call    [red_getopbyte]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

Z_LD_BcbHLb     mov     edi,edx         ;                       LD B,(HL)
                call    mem_rd_di
                xchg    eax,edi
                mov     bh,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_CcbHLb     mov     edi,edx         ;                       LD C,(HL)
                call    mem_rd_di
                xchg    eax,edi
                mov     bl,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_DcbHLb     mov     edi,edx         ;                       LD D,(HL)                                                   LD D,(HL)
                call    mem_rd_di
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_EcbHLb     mov     edi,edx         ;                       LD E,(HL)
                call    mem_rd_di
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_HcbHLb     mov     edi,edx         ;                       LD H,(HL)
                call    mem_rd_di
                xchg    eax,edi
                mov     dh,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_LcbHLb     mov     edi,edx         ;                       LD L,(HL)
                call    mem_rd_di
                xchg    eax,edi
                mov     dl,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_AcbHLb     mov     edi,edx         ;                       LD A,(HL)
                call    mem_rd_di
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                jmp     [red_what_ret]

ZxD_LD_BcbHLb   add     word [red_z80_clk_bus],3 ;              LD B,(Ix+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    eax,edi
                mov     bh,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZxD_LD_CcbHLb   add     word [red_z80_clk_bus],3 ;              LD C,(Ix+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    eax,edi
                mov     bl,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZxD_LD_DcbHLb   add     word [red_z80_clk_bus],3 ;              LD D,(Ix+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZxD_LD_EcbHLb   add     word [red_z80_clk_bus],3 ;              LD E,(Ix+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZDD_LD_HcbHLb   add     word [red_z80_clk_bus],3 ;              LD H,(IX+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    ecx,esi
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                xchg    ecx,esi
                jmp     [red_what_ret]
ZDD_LD_LcbHLb   add     word [red_z80_clk_bus],3 ;              LD L,(IX+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    ecx,esi
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                xchg    ecx,esi
                jmp     [red_what_ret]
ZFD_LD_HcbHLb   add     word [red_z80_clk_bus],3 ;              LD H,(IY+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                ror     esi,0x010
                xchg    ecx,esi
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                xchg    ecx,esi
                rol     esi,0x010
                jmp     [red_what_ret]
ZFD_LD_LcbHLb   add     word [red_z80_clk_bus],3 ;              LD L,(IY+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                ror     esi,0x010
                xchg    ecx,esi
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                xchg    ecx,esi
                rol     esi,0x010
                jmp     [red_what_ret]
ZxD_LD_AcbHLb   add     word [red_z80_clk_bus],3 ;              LD A,(Ix+d)
                get_d_off
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                jmp     [red_what_ret]

Z_LD_bHLbcB     xchg    eax,edi         ;                       LD (HL),B
                mov     al,bh
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bHLbcC     xchg    eax,edi         ;                       LD (HL),C
                mov     al,bl
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bHLbcD     xchg    eax,edi         ;                       LD (HL),D
                mov     al,ch
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bHLbcE     xchg    eax,edi         ;                       LD (HL),E
                mov     al,cl
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bHLbcH     xchg    eax,edi         ;                       LD (HL),H
                mov     al,dh
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bHLbcL     xchg    eax,edi         ;                       LD (HL),L
                mov     al,dl
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
Z_LD_bHLbcA     xchg    ebx,edi         ;                       LD (HL),A
                mov     bl,al
                xchg    ebx,edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]

ZxD_LD_bHLbcB:  add     word [red_z80_clk_bus],3 ;                 LD (Ix+d),B
                get_d_off
                xchg    eax,edi
                mov     al,bh
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxD_LD_bHLbcC:  add     word [red_z80_clk_bus],3 ;                 LD (Ix+d),C
                get_d_off
                xchg    eax,edi
                mov     al,bl
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxD_LD_bHLbcD:  add     word [red_z80_clk_bus],3 ;                 LD (Ix+d),D
                get_d_off
                xchg    eax,edi
                mov     al,ch
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxD_LD_bHLbcE:  add     word [red_z80_clk_bus],3 ;                 LD (Ix+d),E
                get_d_off
                xchg    eax,edi
                mov     al,cl
                xchg    eax,edi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDD_LD_bHLbcH:  add     word [red_z80_clk_bus],3 ;                 LD (IX+d),H
                get_d_off
                xchg    ecx,esi
                xchg    eax,edi
                mov     al,ch
                xchg    eax,edi
                xchg    ecx,esi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDD_LD_bHLbcL:  add     word [red_z80_clk_bus],3 ;                 LD (IX+d),L
                get_d_off
                xchg    ecx,esi
                xchg    eax,edi
                mov     al,cl
                xchg    eax,edi
                xchg    ecx,esi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFD_LD_bHLbcH:  add     word [red_z80_clk_bus],3 ;                 LD (IY+d),H
                get_d_off
                ror     esi,0x010
                xchg    ecx,esi
                xchg    eax,edi
                mov     al,ch
                xchg    eax,edi
                xchg    ecx,esi
                rol     esi,0x010
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFD_LD_bHLbcL:  add     word [red_z80_clk_bus],3 ;                 LD (IY+d),L
                get_d_off
                ror     esi,0x010
                xchg    ecx,esi
                xchg    eax,edi
                mov     al,cl
                xchg    eax,edi
                xchg    ecx,esi
                rol     esi,0x010
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxD_LD_bHLbcA:  add     word [red_z80_clk_bus],3 ;                 LD (Ix+d),A
                get_d_off
                xchg    ebx,edi
                mov     bl,al
                xchg    ebx,edi
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZED_LD_IcA:     add     word [red_z80_clk_bus],1 ;                 LD I,A
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                jmp     [red_what_ret]
ZED_LD_RcA:     add     word [red_z80_clk_bus],1 ;                 LD R,A
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                jmp     [red_what_ret]

ZED_LD_AcI      add     word [red_z80_clk_bus],1 ;                 LD A,I
                ror     edx,0x010
                mov     al,dh
                rol     edx,0x010
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                ror     ebx,0x010
                ror     bx,0x008
                rol     ebx,0x008
                or      bl,bh
                add     al,0
                lahf
                and     eax,0x0ffff40ff
                or      ah,bl
                mov     ebx,edi
                jmp     [red_what_ret]
ZED_LD_AcR      add     word [red_z80_clk_bus],1 ;                 LD A,R
                ror     edx,0x010
                mov     al,dl
                rol     edx,0x010
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                ror     ebx,0x010
                ror     bx,0x008
                rol     ebx,0x008
                or      bl,bh
                add     al,0
                lahf
                and     eax,0x0ffff40ff
                or      ah,bl
                mov     ebx,edi
                jmp     [red_what_ret]

; 16-bit Load opcodes

Z_LD_BCcnn      call    [red_getopbyte] ;                       LD BC,nn
                xchg    eax,edi
                mov     bl,al
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                mov     bh,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_DEcnn      call    [red_getopbyte] ;                       LD DE,nn
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_HLcnn      call    [red_getopbyte]     ;                       LD HL,nn
                xchg    eax,edi
                mov     dl,al
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                mov     dh,al
                xchg    eax,edi
                jmp     [red_what_ret]
Z_LD_SPcnn      call    [red_getopbyte]     ;                       LD SP,nn
                xchg    eax,edi
                ror     ecx,0x010
                mov     cl,al
                rol     ecx,0x010
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                ror     ecx,0x010
                mov     ch,al
                rol     ecx,0x010
                xchg    eax,edi
                jmp     [red_what_ret]

Z_LD_SPcHL      add     word [red_z80_clk_bus],2 ;                 LD SP,HL
                ror     ecx,0x010
                mov     cx,dx
                rol     ecx,0x010
                jmp     [red_what_ret]

ZED_LD_bnnbcBC  call    [red_getopbyte]     ;                       LD (nn),BC
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                rol     edi,0x010
                push    edi
                mov     di,bx
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                pop     edi
                add     edi,0x00010000
                mov     di,bx
                ror     di,0x008
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]
ZED_LD_bnnbcDE  call    [red_getopbyte]     ;                       LD (nn),DE
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                rol     edi,0x010
                push    edi
                mov     di,cx
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                pop     edi
                add     edi,0x00010000
                mov     di,cx
                ror     di,0x008
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]
ZED_LD_bnnbcHL:                         ;                       LD (nn),HL
Z_LD_bnnbcHL    call    [red_getopbyte]
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                rol     edi,0x010
                push    edi
                mov     di,dx
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                pop     edi
                add     edi,0x00010000
                mov     di,dx
                ror     di,0x008
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]
ZED_LD_bnnbcSP  call    [red_getopbyte]     ;                       LD (nn),SP
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                rol     edi,0x010
                push    edi
                ror     ecx,0x010
                mov     di,cx
                rol     ecx,0x010
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                pop     edi
                add     edi,0x00010000
                ror     ecx,0x010
                mov     di,cx
                rol     ecx,0x010
                ror     di,0x008
                mov     [red_xx_32_store],edi
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]

ZED_LD_BCcbnnb  call    [red_getopbyte]     ;                       LD BC,(nn)
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                push    edi
                call    mem_rd_di
                xchg    eax,edi
                mov     bl,al
                xchg    eax,edi
                pop     edi
                inc     edi
                call    mem_rd_di
                xchg    eax,edi
                mov     bh,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZED_LD_DEcbnnb  call    [red_getopbyte]     ;                       LD DE,(nn)
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                push    edi
                call    mem_rd_di
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                pop     edi
                inc     edi
                call    mem_rd_di
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZED_LD_HLcbnnb:                         ;                       LD HL,(nn)
Z_LD_HLcbnnb    call    [red_getopbyte]
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                push    edi
                call    mem_rd_di
                xchg    eax,edi
                mov     dl,al
                xchg    eax,edi
                pop     edi
                inc     edi
                call    mem_rd_di
                xchg    eax,edi
                mov     dh,al
                xchg    eax,edi
                jmp     [red_what_ret]
ZED_LD_SPcbnnb  call    [red_getopbyte]     ;                       LD SP,(nn)
                xchg    eax,edi
                xchg    al,[red_temp_8_store]
                xchg    eax,edi
                call    [red_getopbyte]
                xchg    eax,edi
                rol     eax,0x008
                mov     al,[red_temp_8_store]
                xchg    eax,edi
                push    edi
                call    mem_rd_di
                xchg    eax,edi
                ror     ecx,0x010
                mov     cl,al
                rol     ecx,0x010
                xchg    eax,edi
                pop     edi
                inc     edi
                call    mem_rd_di
                xchg    eax,edi
                ror     ecx,0x010
                mov     ch,al
                rol     ecx,0x010
                xchg    eax,edi
                jmp     [red_what_ret]

; Block load opcodes

ZED_LDD         add     word [red_z80_clk_bus],2 ;                 LDD
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],ecx
                call    mem_wr_di
                dec     bx
                dec     cx
                dec     dx
                xchg    ecx,[red_temp_32_store]
                add     cl,al
                mov     ch,cl
                and     ecx,0x000000208
                and     eax,0x0ffffc1ff
                rol     ch,0x004
                or      ah,cl
                or      ah,ch
                cmp     bx,0
                je      short ldd_skip
                or      eax,0x000000400
ldd_skip:       mov     ecx,[red_temp_32_store]
                jmp     [red_what_ret]
ZED_LDDR        add     word [red_z80_clk_bus],2 ;                 LDDR
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],ecx
                call    mem_wr_di
                dec     bx
                dec     cx
                dec     dx
                xchg    ecx,[red_temp_32_store]
                add     cl,al
                mov     ch,cl
                and     ecx,0x000000208
                and     eax,0x0ffffc1ff
                rol     ch,0x004
                or      ah,cl
                or      ah,ch
                cmp     bx,0
                je      short lddr_skip
                or      eax,0x000000400
                ror     ebx,0x010
                sub     bx,0x002
                rol     ebx,0x010
                add     word [red_z80_clk_bus],5
lddr_skip:      mov     ecx,[red_temp_32_store]
                jmp     [red_what_ret]

ZED_LDI         add     word [red_z80_clk_bus],2 ;                 LDI
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],ecx
                call    mem_wr_di
                dec     bx
                inc     cx
                inc     dx
                xchg    ecx,[red_temp_32_store]
                add     cl,al
                mov     ch,cl
                and     ecx,0x000000208
                and     eax,0x0ffffc1ff
                rol     ch,0x004
                or      ah,cl
                or      ah,ch
                cmp     bx,0
                je      short ldi_skip
                or      eax,0x000000400
ldi_skip:       mov     ecx,[red_temp_32_store]
                jmp     [red_what_ret]
ZED_LDIR        add     word [red_z80_clk_bus],2 ;                 LDIR
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],ecx
                call    mem_wr_di
                dec     bx
                inc     cx
                inc     dx
                xchg    ecx,[red_temp_32_store]
                add     cl,al
                mov     ch,cl
                and     ecx,0x000000208
                and     eax,0x0ffffc1ff
                rol     ch,0x004
                or      ah,cl
                or      ah,ch
                cmp     bx,0
                je      short ldir_skip
                or      eax,0x000000400
                ror     ebx,0x010
                sub     bx,0x002
                rol     ebx,0x010
                add     word [red_z80_clk_bus],5
ldir_skip:      mov     ecx,[red_temp_32_store]
                jmp     [red_what_ret]

; Exchange opcodes

Z_EXX           xchg    bx,[red_loc_BCx] ;                      EXX
                xchg    cx,[red_loc_DEx]
                xchg    dx,[red_loc_HLx]
                jmp     [red_what_ret]
ZDD_EXX         xchg    bx,[red_loc_BCx] ;                      DD EXX
                xchg    cx,[red_loc_DEx]
                xchg    si,[red_loc_HLx]
                jmp     [red_what_ret]
ZFD_EXX         xchg    bx,[red_loc_BCx] ;                      FD EXX
                xchg    cx,[red_loc_DEx]
                ror     esi,0x010
                xchg    si,[red_loc_HLx]
                rol     esi,0x010
                jmp     [red_what_ret]

Z_EX_DEcHL      xchg    cx,dx           ;                       EX DE,HL
                jmp     [red_what_ret]
ZDD_EX_DEcHL:   xchg    cx,si           ;                       DD EX DE,HL
                jmp     [red_what_ret]
ZFD_EX_DEcHL:   ror     esi,0x010       ;                       FD EX DE,HL
                xchg    cx,si
                rol     esi,0x010
                jmp     [red_what_ret]

Z_EX_AFcAFp     xchg    ax,[red_loc_FAx] ;                      EX AF,AF'
                jmp     [red_what_ret]

Z_EX_bSPbcHL    add     word [red_z80_clk_bus],3 ;              EX (SP),HL
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                xchg    dl,al
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                inc     cx
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                xchg    dh,al
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                jmp     [red_what_ret]

; Stack opcodes

Z_POP_BC        ror     ecx,0x010       ;                       POP BC
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                mov     bl,al
                xchg    eax,edi
                add     ecx,0x000010000
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                mov     bh,al
                xchg    eax,edi
                add     ecx,0x000010000
                jmp     [red_what_ret]
Z_POP_DE        ror     ecx,0x010       ;                       POP DE
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                mov     cl,al
                xchg    eax,edi
                add     ecx,0x000010000
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                mov     ch,al
                xchg    eax,edi
                add     ecx,0x000010000
                jmp     [red_what_ret]
Z_POP_HL        ror     ecx,0x010       ;                       POP HL
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                mov     dl,al
                xchg    eax,edi
                add     ecx,0x000010000
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                mov     dh,al
                xchg    eax,edi
                add     ecx,0x000010000
                jmp     [red_what_ret]
Z_POP_AF        ror     ecx,0x010       ;                       POP AF
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    ebx,edi
                mov     ah,bl
                xchg    ebx,edi
                add     ecx,0x000010000
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                add     ecx,0x000010000
                jmp     [red_what_ret]

Z_PUSH_BC       ror     ecx,0x010       ;                       PUSH BC
                dec     cx
                rol     ecx,0x010
                xchg    eax,edi
                mov     al,bh
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                xchg    eax,edi
                mov     al,bl
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]
Z_PUSH_DE       ror     ecx,0x010       ;                       PUSH DE
                dec     cx
                rol     ecx,0x010
                xchg    eax,edi
                mov     al,ch
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                xchg    eax,edi
                mov     al,cl
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]
Z_PUSH_HL       ror     ecx,0x010       ;                       PUSH HL
                dec     cx
                rol     ecx,0x010
                xchg    eax,edi
                mov     al,dh
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                xchg    eax,edi
                mov     al,dl
                xchg    eax,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]
Z_PUSH_AF       ror     ecx,0x010       ;                       PUSH AF
                dec     cx
                rol     ecx,0x010
                xchg    ebx,edi
                mov     bl,al
                xchg    ebx,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                xchg    ebx,edi
                mov     bl,ah
                xchg    ebx,edi
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                jmp     [red_what_ret]

; io opcodes

Z_OUT_bAnbcA    call    [red_getopbyte]     ;                       OUT (An),A
                xchg    edi,ebx
                mov     bh,al
                rol     ebx,0x010
                mov     bl,al
                xchg    edi,ebx
                call    io_wr_di
                jmp     [red_what_ret]

ZED_OUT_bBCbB   xchg    edi,eax         ;                       OUT (BC),B
                mov     ax,bx
                rol     eax,0x010
                mov     al,bh
                xchg    edi,eax
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCbC   xchg    edi,eax         ;                       OUT (BC),C
                mov     ax,bx
                rol     eax,0x010
                mov     al,bl
                xchg    edi,eax
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCbD   xchg    edi,eax         ;                       OUT (BC),D
                mov     ax,bx
                rol     eax,0x010
                mov     al,ch
                xchg    edi,eax
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCbE   xchg    edi,eax         ;                       OUT (BC),E
                mov     ax,bx
                rol     eax,0x010
                mov     al,cl
                xchg    edi,eax
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCbH   xchg    edi,eax         ;                       OUT (BC),H
                mov     ax,bx
                rol     eax,0x010
                mov     al,dh
                xchg    edi,eax
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCbL   xchg    edi,eax         ;                       OUT (BC),L
                mov     ax,bx
                rol     eax,0x010
                mov     al,dl
                xchg    edi,eax
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCb0   xchg    edi,ecx         ;                       OUT (BC),0
                mov     cx,bx
                rol     ecx,0x010
                mov     cl,0
                xchg    edi,ecx
                call    io_wr_di
                jmp     [red_what_ret]
ZED_OUT_bBCbA   xchg    edi,ecx         ;                       OUT (BC),A
                mov     cx,bx
                rol     ecx,0x010
                mov     cl,al
                xchg    edi,ecx
                call    io_wr_di
                jmp     [red_what_ret]

Z_IN_AcbAnb     call    [red_getopbyte]     ;                       IN A,(An)
                xchg    edi,ebx
                mov     bh,al
                xchg    edi,ebx
                call    io_rd_di
                xchg    edi,ebx
                mov     al,bl
                xchg    edi,ebx
                jmp     [red_what_ret]

ZED_IN_BcbBCb   call    io_rd_BC       ;                       IN B,(BC)
                xchg    al,bh
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                xchg    al,bh
                jmp     [red_what_ret]
ZED_IN_CcbBCb   call    io_rd_BC       ;                       IN C,(BC)
                xchg    al,bl
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                xchg    al,bl
                jmp     [red_what_ret]
ZED_IN_DcbBCb   call    io_rd_BC       ;                       IN D,(BC)
                xchg    al,ch
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                xchg    al,ch
                jmp     [red_what_ret]
ZED_IN_EcbBCb   call    io_rd_BC       ;                       IN E,(BC)
                xchg    al,cl
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                xchg    al,cl
                jmp     [red_what_ret]
ZED_IN_HcbBCb   call    io_rd_BC       ;                       IN H,(BC)
                xchg    al,dh
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                xchg    al,dh
                jmp     [red_what_ret]
ZED_IN_LcbBCb   call    io_rd_BC       ;                       IN L,(BC)
                xchg    al,dl
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                xchg    al,dl
                jmp     [red_what_ret]
ZED_IN_bBCb     call    io_rd_BC       ;                       IN (BC)
                mov     [red_temp_8_store],al
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                mov     al,[red_temp_8_store]
                jmp     [red_what_ret]
ZED_IN_AcbBCb   call    io_rd_BC       ;                       IN A,(BC)
                xchg    ebx,edi
                mov     al,bl
                xchg    ebx,edi
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x0040001a8
                or      bh,bl
                add     al,0
                lahf
                and     eax,0x0ffff44ff
                or      ah,bh
                mov     ebx,edi
                jmp     [red_what_ret]

; Block IO opcodes

ZED_IND         add     word [red_z80_clk_bus],1 ;                 IND
                mov     [red_temp_32_storeb],ecx
                call    io_rd_BC
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                dec     bh
                dec     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                dec     cl                  ; add (C-1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,1010010010010010b ; diff for xxIx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short ind_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
ind_skip:       mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_INDR        add     word [red_z80_clk_bus],1 ;                 INDR
                mov     [red_temp_32_storeb],ecx
                call    io_rd_BC
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                dec     bh
                dec     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                dec     cl                  ; add (C-1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,1010010010010010b ; diff for xxIx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short indr_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
                ror     ebx,0x010           ; -+
                sub     bx,0x002            ;  | decrement PC
                rol     ebx,0x010           ; -+
                add   word [red_z80_clk_bus],5 ; more time
indr_skip:      mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_INI         add     word [red_z80_clk_bus],1 ;                 INI
                mov     [red_temp_32_storeb],ecx
                call    io_rd_BC
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                dec     bh
                inc     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                inc     cl                  ; add (C+1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,0110110110100100b ; diff for xxDx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short ini_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
ini_skip:       mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_INIR        add     word [red_z80_clk_bus],1 ;                 INIR
                mov     [red_temp_32_storeb],ecx
                call    io_rd_BC
                mov     [red_temp_32_store],edi
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                dec     bh
                inc     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                inc     cl                  ; add (C+1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,0110110110100100b ; diff for xxDx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short inir_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
                ror     ebx,0x010           ; -+
                sub     bx,0x002            ;  | decrement PC
                rol     ebx,0x010           ; -+
                add   word [red_z80_clk_bus],5 ; more time
inir_skip:      mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_OUTD        add     word [red_z80_clk_bus],1 ;                 OUTD
                mov     [red_temp_32_storeb],ecx
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                call    io_wr_BC
                dec     bh
                dec     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                dec     cl                  ; add (C-1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,1010010010010010b ; diff for xxIx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short outd_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
outd_skip:      mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_OUTDR       add     word [red_z80_clk_bus],1 ;                 OUTDR
                mov     [red_temp_32_storeb],ecx
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                call    io_wr_BC
                dec     bh
                dec     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                dec     cl                  ; add (C-1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,1010010010010010b ; diff for xxIx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short outdr_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
                ror     ebx,0x010           ; -+
                sub     bx,0x002            ;  | decrement PC
                rol     ebx,0x010           ; -+
                add   word [red_z80_clk_bus],5 ; more time
outdr_skip:     mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_OUTI        add     word [red_z80_clk_bus],1 ;                 OUTI
                mov     [red_temp_32_storeb],ecx
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                call    io_wr_BC
                dec     bh
                inc     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                inc     cl                  ; add (C+1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,0110110110100100b ; diff for xxDx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short outi_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
outi_skip:      mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

ZED_OUTIR       add     word [red_z80_clk_bus],1 ;                 OUTIR
                mov     [red_temp_32_storeb],ecx
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],edi
                call    io_wr_BC
                dec     bh
                inc     dx                  ; --------------
                and     eax,0x0ffff00ff     ; wipe old flags
                mov ecx,[red_temp_32_store] ; N flag is bit
                and     ecx,0x000000080     ; 7 of what was
                rol     ecx,0x02            ; read from io
                or      eax,ecx
                mov     ecx,ebx             ; C,H are carry
                and     ecx,0x0000000ff     ; from:
                inc     cl                  ; add (C+1),io
                mov edi,[red_temp_32_store] ; io = what was
                xchg    eax,edi             ; read from io
                add     cl,al
                lahf
                mov     cl,ah
                xchg    eax,edi
                and     ecx,0x000000001
                or      ah,cl
                rol     ecx,0x004
                or      ah,cl
                mov     ecx,ebx             ; S,5,3 are set by B
                and     ecx,0x00000a800     ; as for DEC B op
                or      eax,ecx
                mov edi,[red_temp_32_store] ; P/V is weird...
                xchg    eax,edi             ; ...and this flag
                mov     ch,al               ; hasn't been
                mov     ah,bl               ; tested fully.
                and     ax,0x00303
                rol     ah,0x002
                or      al,ah
                mov     cl,al
                mov     ax,0110110110100100b ; diff for xxDx
                ror     ax,cl
                mov     ah,ch               ; temp1/inp in place
                rol     al,0x002            ; don't mask yet
                rol     eax,0x010           ; store temp
                mov     ecx,ebx
                and     ecx,0x00000f000
                ror     ecx,0x00c
                add     ecx,ioblock
                mov     ax,[ecx]
                mov     ecx,ebx
                and     ecx,0x000000f00
                ror     ecx,0x008
                ror     ax,cl
                mov     ah,bl               ; temp2/c in place
                rol     al,0x002
                and     eax,0x004040404     ; temp2 in al, c2 in ah
                xor     ah,al        ; upper: temp1 in al, inp2 in ah
                ror     eax,0x008
                xor     ah,al
                ror     eax,0x008
                xor     ah,al               ; Finally, done
                mov     ecx,eax
                and     ecx,0x000000400
                xchg    eax,edi
                or      eax,ecx             ; stored P/V
                or      eax,0x000004000     ; Z set if B = 0
                cmp     bh,0                ; --------------
                je      short outir_skip     
                and     eax,0x0ffffbfff     ; Z set if B = 0
                ror     ebx,0x010           ; -+
                sub     bx,0x002            ;  | decrement PC
                rol     ebx,0x010           ; -+
                add   word [red_z80_clk_bus],5 ; more time
outir_skip:     mov     ecx,[red_temp_32_storeb]
                jmp     [red_what_ret]

; Flag control opcodes

Z_SCF           mov     edi,ebx         ;                       SCF
                mov     ebx,eax
                rol     ebx,0x008
                and     ebx,0x000002800
                and     eax,0x0ffffc4ff
                or      eax,ebx
                or      eax,0x000000100
                mov     ebx,edi
                jmp     [red_what_ret]
Z_CCF           test    eax,0x000000100 ;                       CCF
                jz      short Z_SCF
                mov     edi,ebx
                mov     ebx,eax
                rol     ebx,0x008
                and     ebx,0x000002800
                and     eax,0x0ffffc4ff
                or      eax,ebx
                or      eax,0x000001000
                mov     ebx,edi
                jmp     [red_what_ret]

; Program progress control opcodes

Z_JP_HL         ror     ebx,0x010       ;                       JP HL
                mov     bx,dx
                rol     ebx,0x010
                jmp     [red_what_ret]

Z_JP_nn         call    [red_getopbyte]     ;                       JP nn
                mov     [red_temp_32_store],edi
                call    [red_getopbyte]
                xchg    eax,[red_temp_32_store]
                ror     ebx,0x010
                mov     bx,di
                rol     bx,0x008
                mov     bl,al
                rol     ebx,0x010
                xchg    eax,[red_temp_32_store]
                jmp     [red_what_ret]

jump_dummy:     call    [red_getopbyte]
                call    [red_getopbyte]
                jmp     [red_what_ret]

Z_JP_NZcnn      test    ah,0x040        ;                       JP NZ,nn
                jz      short Z_JP_nn
                jmp     short jump_dummy
Z_JP_NCcnn      test    ah,0x001        ;                       JP NC,nn
                jz      short Z_JP_nn
                jmp     short jump_dummy
Z_JP_POcnn      test    ah,0x004        ;                       JP PO,nn
                jz      short Z_JP_nn
                jmp     short jump_dummy
Z_JP_Pcnn       test    ah,0x080        ;                       JP P,nn
                jz      short Z_JP_nn
                jmp     short jump_dummy

Z_JP_Zcnn       test    ah,0x040        ;                       JP Z,nn
                jnz     short Z_JP_nn
                jmp     short jump_dummy
Z_JP_Ccnn       test    ah,0x001        ;                       JP C,nn
                jnz     short Z_JP_nn
                jmp     short jump_dummy
Z_JP_PEcnn      test    ah,0x004        ;                       JP PE,nn
                jnz     short Z_JP_nn
                jmp     short jump_dummy
Z_JP_Mcnn       test    ah,0x080        ;                       JP M,nn
                jnz     short Z_JP_nn
                jmp     short jump_dummy

Z_DJNZ_e        add     word [red_z80_clk_bus],1 ;                 DJNZ e
                dec     bh
                cmp     bh,0
                jne     short Z_JR_e
                jmp     short jr_dummy

Z_JR_e          add     word [red_z80_clk_bus],5 ;                 JR e
                call    [red_getopbyte]
                xchg    eax,edi
                mov     ah,0
                add     al,0
                js      short jr_down
jr_up:          ror     ebx,0x010
                add     bx,ax
                rol     ebx,0x010
                xchg    eax,edi
                jmp     [red_what_ret]
jr_down:        neg     al
                ror     ebx,0x010
                sub     bx,ax
                rol     ebx,0x010
                xchg    eax,edi
                jmp     [red_what_ret]

jr_dummy:       call    [red_getopbyte]
                jmp     [red_what_ret]

Z_JR_NZce       test    ah,0x040        ;                       JR NZ,e
                jz      short Z_JR_e
                jmp     short jr_dummy
Z_JR_NCce       test    ah,0x001        ;                       JR NC,e
                jz      short Z_JR_e
                jmp     short jr_dummy

Z_JR_Zce        test    ah,0x040        ;                       JR Z,e
                jnz     short Z_JR_e
                jmp     short jr_dummy
Z_JR_Cce        test    ah,0x001        ;                       JR C,e
                jnz     short Z_JR_e
                jmp     short jr_dummy

Z_CALL_nn       add     word [red_z80_clk_bus],1 ;                 CALL nn
                call    [red_getopbyte]
                mov     [red_temp_32_store],edi
                call    [red_getopbyte]
                xchg    eax,[red_temp_32_store]
                xchg    ebx,edi
                rol     ebx,0x008
                mov     ah,bh
                xchg    ebx,edi
                xchg    eax,[red_temp_32_store]
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                rol     ebx,0x008
                mov     di,bx
                ror     ebx,0x008
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                rol     ebx,0x010
                mov     di,bx
                ror     ebx,0x010
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ebx,0x010
                xchg    eax,[red_temp_32_store]
                mov     bx,ax
                mov     eax,[red_temp_32_store]
                rol     ebx,0x010
                jmp     [red_what_ret]

call_dummy:     call    [red_getopbyte]
                call    [red_getopbyte]
                jmp     [red_what_ret]

Z_CALL_NZcnn    test    ah,0x040        ;                       CALL NZ,nn
                jz      near Z_CALL_nn
                jmp     short call_dummy
Z_CALL_NCcnn    test    ah,0x001        ;                       CALL NC,nn
                jz      near Z_CALL_nn
                jmp     short call_dummy
Z_CALL_POcnn    test    ah,0x004        ;                       CALL PO,nn
                jz      near Z_CALL_nn
                jmp     short call_dummy
Z_CALL_Pcnn     test    ah,0x080        ;                       CALL P,nn
                jz      near Z_CALL_nn
                jmp     short call_dummy

Z_CALL_Zcnn     test    ah,0x040        ;                       CALL Z,nn
                jnz     near Z_CALL_nn
                jmp     short call_dummy
Z_CALL_Ccnn     test    ah,0x001        ;                       CALL C,nn
                jnz     near Z_CALL_nn
                jmp     short call_dummy
Z_CALL_PEcnn    test    ah,0x004        ;                       CALL PE,nn
                jnz     near Z_CALL_nn
                jmp     short call_dummy
Z_CALL_Mcnn     test    ah,0x080        ;                       CALL M,nn
                jnz     near Z_CALL_nn
                jmp     short call_dummy

%macro          rst_nn  1
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                rol     ebx,0x008
                mov     di,bx
                ror     ebx,0x008
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ecx,0x010
                dec     cx
                rol     ecx,0x010
                rol     ebx,0x010
                mov     di,bx
                ror     ebx,0x010
                mov     [red_xx_32_store],ecx
                ror     dword [red_xx_32_store],0x010
                call    mem_wr_di
                ror     ebx,0x010
                mov     bx,%1
                rol     ebx,0x010
                jmp     [red_what_ret]
%endmacro

Z_RST_00H       rst_nn  0x000000000     ;                       RST 00H
Z_RST_08H       rst_nn  0x000000008     ;                       RST 08H
Z_RST_10H       rst_nn  0x000000010     ;                       RST 10H
Z_RST_18H       rst_nn  0x000000018     ;                       RST 18H
Z_RST_20H       rst_nn  0x000000020     ;                       RST 20H
Z_RST_28H       rst_nn  0x000000028     ;                       RST 28H
Z_RST_30H       rst_nn  0x000000030     ;                       RST 30H
Z_RST_38H       rst_nn  0x000000038     ;                       RST 38H

ZED_RETIN_REAL  inc     word [red_z80_reti_count_bus]  ;        RETI ("real")
ZED_RETIN       ror     ecx,0x010       ;                       RET{I/N}
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                ror     ebx,0x010
                mov     bl,al
                rol     ebx,0x010
                xchg    eax,edi
                add     ecx,0x000010000
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                ror     ebx,0x010
                mov     bh,al
                rol     ebx,0x010
                xchg    eax,edi
                add     ecx,0x000010000
                jmp     ZED_RETIN_RETP  ; This special return point will
                                        ; take care of all the IFF flag
                                        ; stuff.  Hence this is just a
                                        ; standard RET with special flag
                                        ; stuff at the end.
                                        ; NB: This is true of ALL ED based
                                        ;     RETI/N instructions - they
                                        ;     all fix the IFF flags.

Z_RET           ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                ror     ebx,0x010
                mov     bl,al
                rol     ebx,0x010
                xchg    eax,edi
                add     ecx,0x000010000
                ror     ecx,0x010
                mov     edi,ecx
                rol     ecx,0x010
                call    mem_rd_di
                xchg    eax,edi
                ror     ebx,0x010
                mov     bh,al
                rol     ebx,0x010
                xchg    eax,edi
                add     ecx,0x000010000
                jmp     [red_what_ret]

Z_RET_NZ        add     word [red_z80_clk_bus],1 ;                 RET NZ
                test    ah,0x040
                jz      short Z_RET
                jmp     [red_what_ret]
Z_RET_NC        add     word [red_z80_clk_bus],1 ;                 RET NC
                test    ah,0x001
                jz      short Z_RET
                jmp     [red_what_ret]
Z_RET_PO        add     word [red_z80_clk_bus],1 ;                 RET PO
                test    ah,0x004
                jz      near Z_RET
                jmp     [red_what_ret]
Z_RET_P         add     word [red_z80_clk_bus],1 ;                 RET P
                test    ah,0x080
                jz      near Z_RET
                jmp     [red_what_ret]

Z_RET_Z         add     word [red_z80_clk_bus],1 ;                 RET Z
                test    ah,0x040
                jnz     near Z_RET
                jmp     [red_what_ret]
Z_RET_C         add     word [red_z80_clk_bus],1 ;                 RET C
                test    ah,0x001
                jnz     near Z_RET
                jmp     [red_what_ret]
Z_RET_PE        add     word [red_z80_clk_bus],1 ;                 RET PE
                test    ah,0x004
                jnz     near Z_RET
                jmp     [red_what_ret]
Z_RET_M         add     word [red_z80_clk_bus],1 ;                 RET M
                test    ah,0x080
                jnz     near Z_RET
                jmp     [red_what_ret]

; Other operations

; Note: for the NEG op, the C flag will be set by the
;       z80 if A was 0 prior to the operation.  This
;       is the exact inverse of what occurs in the x86.
;       The P/V flag will be set if A was 80 prior
;       to the operation (ie. P/V is acting as overflow,
;       and so does not align with the x86 flags).  The
;       others seem OK (of course, 5 and 3 behaviour is
;       as for Z, and as usual cannot be got from the
;       x86 flags).
;       For some reason, Gaonkar gives another method
;       of finding the carry flag, but as far as I can
;       tell this is wrong, and it certainly disagrees
;       with all other sources I could find.
;
; Update: changed to a LUT for simplicity and accuracy.

ZED_NEG         mov     edi,ebx         ;                       NEG
                mov     ebx,eax
                and     ebx,0x0000000ff
                xchg    ebx,edi
                add     edi,nega
                mov     al,[edi]
                sub     edi,nega
                add     edi,negf
                mov     ah,[edi]
                jmp     [red_what_ret]

Z_RLCA          mov     edi,ebx         ;                       RRCA
                mov     ebx,eax
                mov     bh,bl
                rol     ebx,0x001
                and     ebx,0x000000128
                and     eax,0x0ffffc4ff
                or      ah,bl
                or      ah,bh
                rol     al,0x001
                mov     ebx,edi
                jmp     [red_what_ret]
Z_RRCA          mov     edi,ebx         ;                       RRCA
                mov     ebx,eax
                mov     bh,bl
                ror     ebx,0x001
                and     ebx,0x000002880
                ror     bl,0x007
                and     eax,0x0ffffc4ff
                or      ah,bl
                or      ah,bh
                ror     al,0x001
                mov     ebx,edi
                jmp     [red_what_ret]

Z_RLA           mov     edi,ebx         ;                       RLA
                mov     ebx,eax
                rol     bh,0x007
                rol     bx,0x001
                mov     al,bl
                and     ebx,0x000000128
                and     eax,0x0ffffc4ff
                or      ah,bl
                or      ah,bh
                mov     ebx,edi
                jmp     [red_what_ret]
Z_RRA           mov     edi,ebx         ;                       RRA
                mov     ebx,eax
                ror     bx,0x009
                ror     bl,0x007
                mov     al,bh
                and     ebx,0x000002801
                and     eax,0x0ffffc4ff
                or      ah,bl
                or      ah,bh
                mov     ebx,edi
                jmp     [red_what_ret]

ZED_RLD         add     word [red_z80_clk_bus],4 ;                 RLD
                mov     edi,edx
                call    mem_rd_di
                xchg    edi,ebx
                mov     bh,al
                rol     ebx,0x008
                mov     bl,bh
                rol     ebx,0x008
                mov     bl,al
                rol     bx,0x008
                and     ebx,0x0f0f00f0f
                rol     bl,0x004
                or      bl,bh
                ror     ebx,0x010
                ror     bl,0x004
                or      bh,bl
                mov     al,bh
                mov     bx,ax
                add     al,0
                lahf
                and     eax,0x0ffffc4ff
                and     ebx,0x0ffff0128
                or      ah,bh
                or      ah,bl
                rol     ebx,0x010
                xchg    edi,ebx
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZED_RRD         add     word [red_z80_clk_bus],4 ;                 RRD
                mov     edi,edx
                call    mem_rd_di
                xchg    edi,ebx
                mov     bh,al
                rol     ebx,0x008
                mov     bl,bh
                rol     ebx,0x008
                mov     bl,al
                rol     bx,0x008
                and     ebx,0x0f00f0ff0
                ror     bl,0x004
                rol     bh,0x004
                or      bl,bh
                ror     ebx,0x010
                or      bh,bl
                mov     al,bh
                mov     bx,ax
                add     al,0
                lahf
                and     eax,0x0ffffc4ff
                and     ebx,0x0ffff0128
                or      ah,bh
                or      ah,bl
                rol     ebx,0x010
                xchg    edi,ebx
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]

Z_CPL           xor     al,0x0ff        ;                       CPL
                or      eax,0x000001200
                and     eax,0x0ffffd7ff
                mov     edi,ebx
                mov     ebx,eax
                and     ebx,0x000000028
                or      ah,bl
                mov     ebx,edi
                jmp     [red_what_ret]

Z_DAA           mov     edi,ebx         ;                       DAA
                mov     ebx,eax
                and     eax,0x0ffff03ff
                and     ebx,0x000001000
                ror     ebx,0x002
                or      bx,ax
                xchg    ebx,edi
                add     edi,daaa000
                mov     al,[edi]
                sub     edi,daaa000
                add     edi,daaf000
                mov     ah,[edi]
                jmp     [red_what_ret]

; 16-bit ALU opcodes

Z_INC_BC        inc     bx              ;                       INC BC
                jmp     [red_what_ret]
Z_INC_DE        inc     cx              ;                       INC DE
                jmp     [red_what_ret]
Z_INC_HL        inc     dx              ;                       INC HL
                jmp     [red_what_ret]
Z_INC_SP        ror     ecx,0x010       ;                       INC SP
                inc     cx
                rol     ecx,0x010
                jmp     [red_what_ret]

Z_DEC_BC        dec     bx              ;                       DEC BC
                jmp     [red_what_ret]
Z_DEC_DE        dec     cx              ;                       DEC DE
                jmp     [red_what_ret]
Z_DEC_HL        dec     dx              ;                       DEC HL
                jmp     [red_what_ret]
Z_DEC_SP        ror     ecx,0x010       ;                       DEC SP
                dec     cx
                rol     ecx,0x010
                jmp     [red_what_ret]

Z_ADD_HLcBC     add     word [red_z80_clk_bus],7 ;                 ADD HL,BC
                add     dl,bl
                adc     dh,bh
                mov     edi,ebx
                xchg    eax,ebx
                lahf
                xchg    eax,ebx
                and     eax,0x0ffffc4ff
                and     ebx,0x000001100
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                jmp     [red_what_ret]
Z_ADD_HLcDE     add     word [red_z80_clk_bus],7 ;                 ADD HL,DE
                add     dl,cl
                adc     dh,ch
                mov     edi,ebx
                xchg    eax,ebx
                lahf
                xchg    eax,ebx
                and     eax,0x0ffffc4ff
                and     ebx,0x000001100
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                jmp     [red_what_ret]
Z_ADD_HLcHL     add     word [red_z80_clk_bus],7 ;                 ADD HL,HL
                add     dl,dl
                adc     dh,dh
                mov     edi,ebx
                xchg    eax,ebx
                lahf
                xchg    eax,ebx
                and     eax,0x0ffffc4ff
                and     ebx,0x000001100
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                jmp     [red_what_ret]
Z_ADD_HLcSP     add     word [red_z80_clk_bus],7 ;                 ADD HL,SP
                ror     ecx,0x010
                add     dl,cl
                adc     dh,ch
                mov     edi,ebx
                xchg    eax,ebx
                lahf
                rol     ecx,0x010 ; this needs to happen after flags are done
                xchg    eax,ebx
                and     eax,0x0ffffc4ff
                and     ebx,0x000001100
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                jmp     [red_what_ret]

ZED_ADC_HLcBC   add     word [red_z80_clk_bus],7 ;                 ADC HL,BC
                sahf
                adc     dl,bl
                adc     dh,bh
                pushfw
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short adc_hlcbc_skip
                or      eax,0x000004000
adc_hlcbc_skip: jmp     [red_what_ret]
ZED_ADC_HLcDE   add     word [red_z80_clk_bus],7 ;                 ADC HL,DE
                sahf
                adc     dl,cl
                adc     dh,ch
                pushfw
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short adc_hlcde_skip
                or      eax,0x000004000
adc_hlcde_skip: jmp     [red_what_ret]
ZED_ADC_HLcHL   add     word [red_z80_clk_bus],7 ;                 ADC HL,HL
                sahf
                adc     dl,dl
                adc     dh,dh
                pushfw
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short adc_hlchl_skip
                or      eax,0x000004000
adc_hlchl_skip: jmp     [red_what_ret]
ZED_ADC_HLcSP   add     word [red_z80_clk_bus],7 ;                 ADC HL,SP
                ror     ecx,0x010
                sahf
                adc     dl,cl
                adc     dh,ch
                pushfw
                rol     ecx,0x010
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short adc_hlcsp_skip
                or      eax,0x000004000
adc_hlcsp_skip: jmp     [red_what_ret]

ZED_SBC_HLcBC   add     word [red_z80_clk_bus],7 ;                 SBC HL,BC
                sahf
                sbb     dl,bl
                sbb     dh,bh
                pushfw
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short sbc_hlcbc_skip
                or      eax,0x000004000
sbc_hlcbc_skip: or      eax,0x000000200
                jmp     [red_what_ret]
ZED_SBC_HLcDE   add     word [red_z80_clk_bus],7 ;                 SBC HL,DE
                sahf
                sbb     dl,cl
                sbb     dh,ch
                pushfw
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short sbc_hlcde_skip
                or      eax,0x000004000
sbc_hlcde_skip: or      eax,0x000000200
                jmp     [red_what_ret]
ZED_SBC_HLcHL   add     word [red_z80_clk_bus],7 ;                 SBC HL,HL
                sahf
                sbb     dl,dl
                sbb     dh,dh
                pushfw
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short sbc_hlchl_skip
                or      eax,0x000004000
sbc_hlchl_skip: or      eax,0x000000200
                jmp     [red_what_ret]
ZED_SBC_HLcSP   add     word [red_z80_clk_bus],7 ;                 SBC HL,SP
                ror     ecx,0x010
                sahf
                sbb     dl,cl
                sbb     dh,ch
                pushfw
                rol     ecx,0x010
                mov     edi,ebx
                pop     bx
                and     eax,0x0ffff00ff
                and     ebx,0x000000891
                ror     bh,0x001
                or      bh,bl
                mov     bl,0
                or      eax,ebx
                mov     ebx,edx
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
                cmp     dx,0
                jne     short sbc_hlcsp_skip
                or      eax,0x000004000
sbc_hlcsp_skip: or      eax,0x000000200
                jmp     [red_what_ret]

; 8-bit ALU opcodes

%macro          get_flag_SZ5H3V0C 0
                mov     edi,ebx
                pushfw
                pop     bx
                and     ebx,0x0000008d1
                ror     bh,0x001
                or      bh,bl
                mov     bl,al
                and     ebx,0x00000d728
                or      bh,bl
                mov     ah,bh
                mov     ebx,edi
%endmacro

%macro          get_flag_SZ5H3V1C 0
                mov     edi,ebx
                pushfw
                pop     bx
                and     ebx,0x0000008d1
                ror     bh,0x001
                or      bh,bl
                or      ebx,0x000000200
                mov     bl,al
                and     ebx,0x00000d728
                or      bh,bl
                mov     ah,bh
                mov     ebx,edi
%endmacro

%macro          get_flag_SZ513P00 0
                mov     edi,ebx
                lahf
                and     eax,0x0ffffc4ff
                or      eax,0x000001000
                mov     bh,al
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
%endmacro

%macro          get_flag_SZ503P00 0
                mov     edi,ebx
                lahf
                and     eax,0x0ffffc4ff
                mov     bh,al
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
%endmacro

%macro          get_flag_SZ503P0C 0
                mov     edi,ebx
                lahf
                and     eax,0x0ffffc5ff
                mov     bh,al
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
%endmacro

Z_INC_B         sahf                    ;                       INC B
                xchg    al,bh
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,bh
                jmp     [red_what_ret]
Z_INC_C         sahf                    ;                       INC C
                xchg    al,bl
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,bl
                jmp     [red_what_ret]
Z_INC_D         sahf                    ;                       INC D
                xchg    al,ch
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,ch
                jmp     [red_what_ret]
Z_INC_E         sahf                    ;                       INC E
                xchg    al,cl
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,cl
                jmp     [red_what_ret]
Z_INC_H         sahf                    ;                       INC H
                xchg    al,dh
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,dh
                jmp     [red_what_ret]
Z_INC_L         sahf                    ;                       INC L
                xchg    al,dl
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,dl
                jmp     [red_what_ret]
Z_INC_A         sahf                    ;                       INC A
                inc     al
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]

Z_INC_bHLb      add     word [red_z80_clk_bus],1 ;                 INC (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                xchg    al,bl
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxD_INC_bHLb    get_d_off               ;                       INC (HL+d)
                add     word [red_z80_clk_bus],2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                xchg    al,bl
                inc     al
                get_flag_SZ5H3V0C
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

Z_DEC_B         sahf                    ;                       DEC B
                xchg    al,bh
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,bh
                jmp     [red_what_ret]
Z_DEC_C         sahf                    ;                       DEC C
                xchg    al,bl
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,bl
                jmp     [red_what_ret]
Z_DEC_D         sahf                    ;                       DEC D
                xchg    al,ch
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,ch
                jmp     [red_what_ret]
Z_DEC_E         sahf                    ;                       DEC E
                xchg    al,cl
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,cl
                jmp     [red_what_ret]
Z_DEC_H         sahf                    ;                       DEC H
                xchg    al,dh
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,dh
                jmp     [red_what_ret]
Z_DEC_L         sahf                    ;                       DEC L
                xchg    al,dl
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,dl
                jmp     [red_what_ret]
Z_DEC_A         sahf                    ;                       DEC A
                dec     al
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]

Z_DEC_bHLb      add     word [red_z80_clk_bus],1 ;                 DEC (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                xchg    al,bl
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxD_DEC_bHLb    get_d_off               ;                       DEC (HL+d)
                add     word [red_z80_clk_bus],2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                xchg    al,bl
                dec     al
                get_flag_SZ5H3V1C
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

Z_ADD_Acn       call    [red_getopbyte]     ;                       ADD A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                add     al,bl
                get_flag_SZ5H3V0C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_ADD_AcB       add     al,bh           ;                       ADD A,B
                get_flag_SZ5H3V0C       ;  (8 bit addition has been
                jmp     [red_what_ret]      ;   exhaustively verified).
Z_ADD_AcC       add     al,bl           ;                       ADD A,C
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADD_AcD       add     al,ch           ;                       ADD A,D
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADD_AcE       add     al,cl           ;                       ADD A,E
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADD_AcH       add     al,dh           ;                       ADD A,H
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADD_AcL       add     al,dl           ;                       ADD A,L
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADD_AcA       add     al,al           ;                       ADD A,A
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]

Z_ADD_AcbHLb    mov     edi,edx         ;                       ADD A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                add     al,bl
                get_flag_SZ5H3V0C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_ADD_AcbHLb  get_d_off               ;                       ADD A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                add     al,bl
                get_flag_SZ5H3V0C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_SUB_Acn       call    [red_getopbyte]     ;                       SUB A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sub     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_SUB_AcB       sub     al,bh           ;                       SUB A,B
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SUB_AcC       sub     al,bl           ;                       SUB A,C
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SUB_AcD       sub     al,ch           ;                       SUB A,D
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SUB_AcE       sub     al,cl           ;                       SUB A,E
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SUB_AcH       sub     al,dh           ;                       SUB A,H
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SUB_AcL       sub     al,dl           ;                       SUB A,L
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SUB_AcA       sub     al,al           ;                       SUB A,A
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]

Z_SUB_AcbHLb    mov     edi,edx         ;                       SUB A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sub     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_SUB_AcbHLb  get_d_off               ;                       SUB A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sub     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_ADC_Acn       call    [red_getopbyte]     ;                       ADC A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                adc     al,bl
                get_flag_SZ5H3V0C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_ADC_AcB       sahf                    ;                       ADC A,B
                adc     al,bh           ;   (8 bit add with carry has been
                get_flag_SZ5H3V0C       ;    exhaustively verified)
                jmp     [red_what_ret]
Z_ADC_AcC       sahf                    ;                       ADC A,C
                adc     al,bl
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADC_AcD       sahf                    ;                       ADC A,D
                adc     al,ch
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADC_AcE       sahf                    ;                       ADC A,E
                adc     al,cl
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADC_AcH       sahf                    ;                       ADC A,H
                adc     al,dh
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADC_AcL       sahf                    ;                       ADC A,L
                adc     al,dl
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]
Z_ADC_AcA       sahf                    ;                       ADC A,A
                adc     al,al
                get_flag_SZ5H3V0C
                jmp     [red_what_ret]

Z_ADC_AcbHLb    mov     edi,edx         ;                       ADC A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                adc     al,bl
                get_flag_SZ5H3V0C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_ADC_AcbHLb  get_d_off               ;                       ADC A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                adc     al,bl
                get_flag_SZ5H3V0C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_SBC_Acn       call    [red_getopbyte]     ;                       SBC A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                sbb     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_SBC_AcB       sahf                    ;                       SBC A,B
                sbb     al,bh
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SBC_AcC       sahf                    ;                       SBC A,C
                sbb     al,bl
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SBC_AcD       sahf                    ;                       SBC A,D
                sbb     al,ch
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SBC_AcE       sahf                    ;                       SBC A,E
                sbb     al,cl
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SBC_AcH       sahf                    ;                       SBC A,H
                sbb     al,dh
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SBC_AcL       sahf                    ;                       SBC A,L
                sbb     al,dl
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_SBC_AcA       sahf                    ;                       SBC A,A
                sbb     al,al
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]

Z_SBC_AcbHLb    mov     edi,edx         ;                       SBC A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                sbb     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_SBC_AcbHLb  get_d_off               ;                       SBC A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                sahf
                sbb     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_CP_Acn        call    [red_getopbyte]     ;                       CP A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                cmp     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_CP_AcB        cmp     al,bh           ;                       CP A,B
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_CP_AcC        cmp     al,bl           ;                       CP A,C
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_CP_AcD        cmp     al,ch           ;                       CP A,D
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_CP_AcE        cmp     al,cl           ;                       CP A,E
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_CP_AcH        cmp     al,dh           ;                       CP A,H
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_CP_AcL        cmp     al,dl           ;                       CP A,L
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]
Z_CP_AcA        cmp     al,al           ;                       CP A,A
                get_flag_SZ5H3V1C
                jmp     [red_what_ret]

Z_CP_AcbHLb     mov     edi,edx         ;                       CP A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                cmp     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_CP_AcbHLb   get_d_off               ;                       CP A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                cmp     al,bl
                get_flag_SZ5H3V1C
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_AND_Acn       call    [red_getopbyte]     ;                       AND A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                and     al,bl
                get_flag_SZ513P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_AND_AcB       and     al,bh           ;                       AND A,B
                get_flag_SZ513P00
                jmp     [red_what_ret]
Z_AND_AcC       and     al,bl           ;                       AND A,C
                get_flag_SZ513P00
                jmp     [red_what_ret]
Z_AND_AcD       and     al,ch           ;                       AND A,D
                get_flag_SZ513P00
                jmp     [red_what_ret]
Z_AND_AcE       and     al,cl           ;                       AND A,E
                get_flag_SZ513P00
                jmp     [red_what_ret]
Z_AND_AcH       and     al,dh           ;                       AND A,H
                get_flag_SZ513P00
                jmp     [red_what_ret]
Z_AND_AcL       and     al,dl           ;                       AND A,L
                get_flag_SZ513P00
                jmp     [red_what_ret]
Z_AND_AcA       and     al,al           ;                       AND A,A
                get_flag_SZ513P00
                jmp     [red_what_ret]

Z_AND_AcbHLb    mov     edi,edx         ;                       AND A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                and     al,bl
                get_flag_SZ513P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_AND_AcbHLb  get_d_off               ;                       AND A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                and     al,bl
                get_flag_SZ513P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_XOR_Acn       call    [red_getopbyte]     ;                       XOR A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xor     al,bl
                get_flag_SZ503P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_XOR_AcB       xor     al,bh           ;                       XOR A,B
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_XOR_AcC       xor     al,bl           ;                       XOR A,C
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_XOR_AcD       xor     al,ch           ;                       XOR A,D
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_XOR_AcE       xor     al,cl           ;                       XOR A,E
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_XOR_AcH       xor     al,dh           ;                       XOR A,H
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_XOR_AcL       xor     al,dl           ;                       XOR A,L
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_XOR_AcA       xor     al,al           ;                       XOR A,A
                get_flag_SZ503P00
                jmp     [red_what_ret]

Z_XOR_AcbHLb    mov     edi,edx         ;                       XOR A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xor     al,bl
                get_flag_SZ503P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_XOR_AcbHLb  get_d_off               ;                       XOR A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xor     al,bl
                get_flag_SZ503P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_OR_Acn        call    [red_getopbyte]     ;                       OR A,n
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                or      al,bl
                get_flag_SZ503P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]

Z_OR_AcB        or      al,bh           ;                       OR A,B
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_OR_AcC        or      al,bl           ;                       OR A,C
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_OR_AcD        or      al,ch           ;                       OR A,D
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_OR_AcE        or      al,cl           ;                       OR A,E
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_OR_AcH        or      al,dh           ;                       OR A,H
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_OR_AcL        or      al,dl           ;                       OR A,L
                get_flag_SZ503P00
                jmp     [red_what_ret]
Z_OR_AcA        or      al,al           ;                       OR A,A
                get_flag_SZ503P00
                jmp     [red_what_ret]

Z_OR_AcbHLb     mov     edi,edx         ;                       OR A,(HL)
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                or      al,bl
                get_flag_SZ503P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]
ZxD_OR_AcbHLb   get_d_off               ;                       OR A,(HL+d)
                add     word [red_z80_clk_bus],3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                or      al,bl
                get_flag_SZ503P00
                mov     ebx,[red_temp_32_store]
                jmp     [red_what_ret]


; Block ALU opcodes

ZED_CPD         add     word [red_z80_clk_bus],5 ;                 CPD
                mov     edi,edx
                call    mem_rd_di
                xchg    edi,ebx
                mov     bh,al
                xchg    al,bl
                sub     bl,al
                mov     al,bh
                pushfw
                rol     ebx,0x010
                pop     bx
                mov     bh,bl
                and     ebx,0x000ff10d0
                ror     ebx,0x008
                ror     bl,0x004
                sub     bh,bl
                rol     ebx,0x008
                ror     bx,0x008
                ror     ebx,0x008
                and     ebx,0x000000ad0
                or      bh,bl
                mov     bl,bh
                and     ebx,0x00000d802
                rol     bl,0x004
                or      bh,bl
                mov     bl,ah
                and     ebx,0x00000f801
                or      ebx,0x000000002
                or      bh,bl
                mov     ah,bh
                xchg    edi,ebx
                dec     bx
                dec     dx
                cmp     bx,0
                je      short cpd_skip
                or      eax,0x000000400
cpd_skip:       jmp     [red_what_ret]

ZED_CPDR        add     word [red_z80_clk_bus],5 ;                 CPDR
                mov     edi,edx
                call    mem_rd_di
                xchg    edi,ebx
                mov     bh,al
                xchg    al,bl
                sub     bl,al
                mov     al,bh
                pushfw
                rol     ebx,0x010
                pop     bx
                mov     bh,bl
                and     ebx,0x000ff10d0
                ror     ebx,0x008
                ror     bl,0x004
                sub     bh,bl
                rol     ebx,0x008
                ror     bx,0x008
                ror     ebx,0x008
                and     ebx,0x000000ad0
                or      bh,bl
                mov     bl,bh
                and     ebx,0x00000d802
                rol     bl,0x004
                or      bh,bl
                mov     bl,ah
                and     ebx,0x00000f801
                or      ebx,0x000000002
                or      bh,bl
                mov     ah,bh
                xchg    edi,ebx
                dec     bx
                dec     dx
                cmp     bx,0
                je      short cpdr_skip
                or      eax,0x000000400
                sahf
                jz      short cpdr_skip
                ror     ebx,0x010
                sub     bx,0x002
                rol     ebx,0x010
                add     word [red_z80_clk_bus],5
cpdr_skip:      jmp     [red_what_ret]

ZED_CPI         add     word [red_z80_clk_bus],5 ;                 CPI
                mov     edi,edx
                call    mem_rd_di
                xchg    edi,ebx
                mov     bh,al
                xchg    al,bl
                sub     bl,al
                mov     al,bh
                pushfw
                rol     ebx,0x010
                pop     bx
                mov     bh,bl
                and     ebx,0x000ff10d0
                ror     ebx,0x008
                ror     bl,0x004
                sub     bh,bl
                rol     ebx,0x008
                ror     bx,0x008
                ror     ebx,0x008
                and     ebx,0x000000ad0
                or      bh,bl
                mov     bl,bh
                and     ebx,0x00000d802
                rol     bl,0x004
                or      bh,bl
                mov     bl,ah
                and     ebx,0x00000f801
                or      ebx,0x000000002
                or      bh,bl
                mov     ah,bh
                xchg    edi,ebx
                dec     bx
                inc     dx
                cmp     bx,0
                je      short cpi_skip
                or      eax,0x000000400
cpi_skip:       jmp     [red_what_ret]

ZED_CPIR        add     word [red_z80_clk_bus],5 ;                 CPIR
                mov     edi,edx
                call    mem_rd_di
                xchg    edi,ebx
                mov     bh,al
                xchg    al,bl
                sub     bl,al
                mov     al,bh
                pushfw
                rol     ebx,0x010
                pop     bx
                mov     bh,bl
                and     ebx,0x000ff10d0
                ror     ebx,0x008
                ror     bl,0x004
                sub     bh,bl
                rol     ebx,0x008
                ror     bx,0x008
                ror     ebx,0x008
                and     ebx,0x000000ad0
                or      bh,bl
                mov     bl,bh
                and     ebx,0x00000d802
                rol     bl,0x004
                or      bh,bl
                mov     bl,ah
                and     ebx,0x00000f801
                or      ebx,0x000000002
                or      bh,bl
                mov     ah,bh
                xchg    edi,ebx
                dec     bx
                inc     dx
                cmp     bx,0
                je      short cpir_skip
                or      eax,0x000000400
                sahf
                jz      short cpir_skip
                ror     ebx,0x010
                sub     bx,0x002
                rol     ebx,0x010
                add     word [red_z80_clk_bus],5
cpir_skip:      jmp     [red_what_ret]


;***********************************************************************;
;***********************************************************************;
;                                                                       ;
;***********************************************************************;
;                                                                       ;
; CB opcodes start here                                                 ;
;                                                                       ;
;***********************************************************************;
;                                                                       ;
;***********************************************************************;
;***********************************************************************;

%macro          RLC_A 0
                rol     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RLC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RLC:
%endmacro

%macro          RRC_A 0
                ror     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RRC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RRC:
%endmacro

%macro          RL_A 0
                sahf
                rcl     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RLC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RLC:
%endmacro

%macro          RR_A 0
                sahf
                rcr     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RRC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RRC:
%endmacro

%macro          SLA_A 0
                sal     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RLC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RLC:
%endmacro

%macro          SRA_A 0
                sar     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RRC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RRC:
%endmacro

%macro          SLL_A 0
                shl     al,0x001
                or      eax,0x000000001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RLC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RLC:
%endmacro

%macro          SRL_A 0
                shr     al,0x001
                jc      short %%is_carry
%%no_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                jmp     short %%end_RRC
%%is_carry:     add     al,0x000
                lahf
                get_flag_SZ503P00
                or      eax,0x000000100
%%end_RRC:
%endmacro

%macro          RES_Acx 1
                mov     edi,ebx
                mov     ebx,0x0fffffffe
                rol     ebx,%1
                and     eax,ebx
                mov     ebx,edi
%endmacro

%macro          SET_Acx 1
                mov     edi,ebx
                mov     ebx,0x000000001
                rol     ebx,%1
                or      eax,ebx
                mov     ebx,edi
%endmacro

%macro          BIT_Acx 1
                mov     edi,ebx
                mov     ebx,0x000000001
                rol     ebx,%1
                mov     bh,al
                and     bh,bl
                jz      %%BIT_zero
%%BIT_one:      mov     bl,ah
                and     eax,0x0ffff00ff
                and     ebx,0x00000a801
                or      eax,0x000001000
                or      bh,bl
                and     ebx,0x00000a900
                or      eax,ebx
                jmp     short %%BIT_done
%%BIT_zero:     mov     bl,ah
                and     eax,0x0ffff00ff
                and     ebx,0x00000a801
                or      eax,0x000005400
                or      bh,bl
                and     ebx,0x00000a900
                or      eax,ebx
%%BIT_done:     mov     ebx,edi
%endmacro

; The flag operation of the BIT opcode is different for memory BIT
; operations.

%macro          BITdAcx 1
                mov     edi,ebx
                mov     ebx,0x000000001
                rol     ebx,%1
                mov     bh,al
                and     bh,bl
                jz      %%BIT_zero
%%BIT_one:      mov     bl,ah
                and     eax,0x0ffff00ff
                and     ebx,0x000008001
                or      eax,0x000001000
                or      bh,bl
                and     ebx,0x000008100
                or      eax,ebx
                jmp     short %%BIT_done
%%BIT_zero:     mov     bl,ah
                and     eax,0x0ffff00ff
                and     ebx,0x000008001
                or      eax,0x000005400
                or      bh,bl
                and     ebx,0x000008100
                or      eax,ebx
%%BIT_done:     mov     bx,[red_hidden_reg]
                and     ebx,0x000002800
                or      eax,ebx
                mov     ebx,edi
%endmacro




ZCB_RLC_B       xchg    al,bh           ;                       RLC B
                RLC_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RLC_C       xchg    al,bl           ;                       RLC C
                RLC_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RLC_D       xchg    al,ch           ;                       RLC D
                RLC_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RLC_E       xchg    al,cl           ;                       RLC E
                RLC_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RLC_H       xchg    al,dh           ;                       RLC H
                RLC_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RLC_L       xchg    al,dl           ;                       RLC L
                RLC_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RLC_A       RLC_A                   ;                       RLC A
                jmp     [red_what_ret]

ZCB_RLC_bHLb    add     word [red_z80_clk_bus],1 ;                 RLC (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RLC_bHLb  sub     word [red_z80_clk_bus],2 ;                 RLC (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RLC_B     sub     word [red_z80_clk_bus],2 ;                 RLC (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RLC_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RLC_C     sub     word [red_z80_clk_bus],2 ;                 RLC (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RLC_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RLC_D     sub     word [red_z80_clk_bus],2 ;                 RLC (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RLC_E     sub     word [red_z80_clk_bus],2 ;                 RLC (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RLC_H     sub     word [red_z80_clk_bus],2 ;                 RLC (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RLC_L     sub     word [red_z80_clk_bus],2 ;                   RLC (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RLC_H     sub     word [red_z80_clk_bus],2 ;                   RLC (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RLC_L     sub     word [red_z80_clk_bus],2 ;                   RLC (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RLC_A     sub     word [red_z80_clk_bus],2 ;                   RLC (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RLC_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RRC_B       xchg    al,bh           ;                       RRC B
                RRC_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RRC_C       xchg    al,bl           ;                       RRC C
                RRC_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RRC_D       xchg    al,ch           ;                       RRC D
                RRC_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RRC_E       xchg    al,cl           ;                       RRC E
                RRC_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RRC_H       xchg    al,dh           ;                       RRC H
                RRC_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RRC_L       xchg    al,dl           ;                       RRC L
                RRC_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RRC_A       RRC_A                   ;                       RRC A
                jmp     [red_what_ret]

ZCB_RRC_bHLb    add     word [red_z80_clk_bus],1 ;                   RRC (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RRC_bHLb  sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RRC_B     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RRC_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RRC_C     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RRC_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RRC_D     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RRC_E     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RRC_H     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RRC_L     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RRC_H     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RRC_L     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RRC_A     sub     word [red_z80_clk_bus],2 ;                   RRC (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RRC_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RL_B        xchg    al,bh           ;                       RL B
                RL_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RL_C        xchg    al,bl           ;                       RL C
                RL_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RL_D        xchg    al,ch           ;                       RL D
                RL_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RL_E        xchg    al,cl           ;                       RL E
                RL_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RL_H        xchg    al,dh           ;                       RL H
                RL_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RL_L        xchg    al,dl           ;                       RL L
                RL_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RL_A        RL_A                   ;                       RL A
                jmp     [red_what_ret]

ZCB_RL_bHLb     add     word [red_z80_clk_bus],1 ;                   RL (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RL_bHLb   sub     word [red_z80_clk_bus],2 ;                   RL (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RL_B      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RL_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RL_C      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RL_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RL_D      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RL_E      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RL_H      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RL_L      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RL_H      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RL_L      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RL_A      sub     word [red_z80_clk_bus],2 ;                   RL (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RL_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RR_B        xchg    al,bh           ;                       RR B
                RR_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RR_C        xchg    al,bl           ;                       RR C
                RR_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RR_D        xchg    al,ch           ;                       RR D
                RR_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RR_E        xchg    al,cl           ;                       RR E
                RR_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RR_H        xchg    al,dh           ;                       RR H
                RR_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RR_L        xchg    al,dl           ;                       RR L
                RR_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RR_A        RR_A                   ;                       RR A
                jmp     [red_what_ret]

ZCB_RR_bHLb     add     word [red_z80_clk_bus],1 ;                   RR (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RR_bHLb   sub     word [red_z80_clk_bus],2 ;                   RR (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RR_B      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RR_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RR_C      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RR_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RR_D      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RR_E      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RR_H      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RR_L      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RR_H      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RR_L      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RR_A      sub     word [red_z80_clk_bus],2 ;                   RR (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RR_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SLA_B       xchg    al,bh           ;                       SLA B
                SLA_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SLA_C       xchg    al,bl           ;                       SLA C
                SLA_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SLA_D       xchg    al,ch           ;                       SLA D
                SLA_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SLA_E       xchg    al,cl           ;                       SLA E
                SLA_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SLA_H       xchg    al,dh           ;                       SLA H
                SLA_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SLA_L       xchg    al,dl           ;                       SLA L
                SLA_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SLA_A       SLA_A                   ;                       SLA A
                jmp     [red_what_ret]

ZCB_SLA_bHLb    add     word [red_z80_clk_bus],1 ;                   SLA (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLA_bHLb  sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SLA_B     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SLA_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLA_C     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SLA_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLA_D     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLA_E     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SLA_H     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SLA_L     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SLA_H     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SLA_L     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLA_A     sub     word [red_z80_clk_bus],2 ;                   SLA (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLA_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SRA_B       xchg    al,bh           ;                       SRA B
                SRA_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SRA_C       xchg    al,bl           ;                       SRA C
                SRA_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SRA_D       xchg    al,ch           ;                       SRA D
                SRA_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SRA_E       xchg    al,cl           ;                       SRA E
                SRA_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SRA_H       xchg    al,dh           ;                       SRA H
                SRA_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SRA_L       xchg    al,dl           ;                       SRA L
                SRA_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SRA_A       SRA_A                   ;                       SRA A
                jmp     [red_what_ret]

ZCB_SRA_bHLb    add     word [red_z80_clk_bus],1 ;                   SRA (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRA_bHLb  sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SRA_B     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SRA_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRA_C     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SRA_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRA_D     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRA_E     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SRA_H     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SRA_L     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SRA_H     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SRA_L     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRA_A     sub     word [red_z80_clk_bus],2 ;                   SRA (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRA_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SLL_B       xchg    al,bh           ;                       SLL B
                SLL_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SLL_C       xchg    al,bl           ;                       SLL C
                SLL_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SLL_D       xchg    al,ch           ;                       SLL D
                SLL_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SLL_E       xchg    al,cl           ;                       SLL E
                SLL_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SLL_H       xchg    al,dh           ;                       SLL H
                SLL_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SLL_L       xchg    al,dl           ;                       SLL L
                SLL_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SLL_A       SLL_A                   ;                       SLL A
                jmp     [red_what_ret]

ZCB_SLL_bHLb    add     word [red_z80_clk_bus],1 ;                   SLL (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLL_bHLb  sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SLL_B     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SLL_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLL_C     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SLL_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLL_D     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLL_E     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SLL_H     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SLL_L     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SLL_H     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SLL_L     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SLL_A     sub     word [red_z80_clk_bus],2 ;                   SLL (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SLL_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SRL_B       xchg    al,bh           ;                       SRL B
                SRL_A
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SRL_C       xchg    al,bl           ;                       SRL C
                SRL_A
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SRL_D       xchg    al,ch           ;                       SRL D
                SRL_A
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SRL_E       xchg    al,cl           ;                       SRL E
                SRL_A
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SRL_H       xchg    al,dh           ;                       SRL H
                SRL_A
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SRL_L       xchg    al,dl           ;                       SRL L
                SRL_A
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SRL_A       SRL_A                   ;                       SRL A
                jmp     [red_what_ret]

ZCB_SRL_bHLb    add     word [red_z80_clk_bus],1 ;                   SRL (HL)
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRL_bHLb  sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d)
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SRL_B     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),B
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SRL_A
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRL_C     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),C
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SRL_A
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRL_D     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),D
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRL_E     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),E
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SRL_H     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),IXh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SRL_L     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),IXl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SRL_H     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),IYh
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SRL_L     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),IYl
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SRL_A     sub     word [red_z80_clk_bus],2 ;                   SRL (HL+d),A
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SRL_A
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc0     xchg    al,bh           ;                       RES B,0
                RES_Acx 0
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc0     xchg    al,bl           ;                       RES C,0
                RES_Acx 0
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc0     xchg    al,ch           ;                       RES D,0
                RES_Acx 0
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec0     xchg    al,cl           ;                       RES E,0
                RES_Acx 0
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc0     xchg    al,dh           ;                       RES H,0
                RES_Acx 0
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc0     xchg    al,dl           ;                       RES L,0
                RES_Acx 0
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac0     RES_Acx 0               ;                       RES A,0
                jmp     [red_what_ret]

ZCB_RES_bHLbc0  add     word [red_z80_clk_bus],1 ;                   RES (HL),0
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc0 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 0
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 0
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac0   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 0
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc1     xchg    al,bh           ;                       RES B,1
                RES_Acx 1
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc1     xchg    al,bl           ;                       RES C,1
                RES_Acx 1
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc1     xchg    al,ch           ;                       RES D,1
                RES_Acx 1
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec1     xchg    al,cl           ;                       RES E,1
                RES_Acx 1
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc1     xchg    al,dh           ;                       RES H,1
                RES_Acx 1
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc1     xchg    al,dl           ;                       RES L,1
                RES_Acx 1
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac1     RES_Acx 1               ;                       RES A,1
                jmp     [red_what_ret]

ZCB_RES_bHLbc1  add     word [red_z80_clk_bus],1 ;                   RES (HL),1
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc1 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 1
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 1
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac1   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 1
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc2     xchg    al,bh           ;                       RES B,2
                RES_Acx 2
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc2     xchg    al,bl           ;                       RES C,2
                RES_Acx 2
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc2     xchg    al,ch           ;                       RES D,2
                RES_Acx 2
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec2     xchg    al,cl           ;                       RES E,2
                RES_Acx 2
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc2     xchg    al,dh           ;                       RES H,2
                RES_Acx 2
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc2     xchg    al,dl           ;                       RES L,2
                RES_Acx 2
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac2     RES_Acx 2               ;                       RES A,2
                jmp     [red_what_ret]

ZCB_RES_bHLbc2  add     word [red_z80_clk_bus],1 ;                   RES (HL),2
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc2 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 2
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 2
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac2   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 2
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc3     xchg    al,bh           ;                       RES B,3
                RES_Acx 3
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc3     xchg    al,bl           ;                       RES C,3
                RES_Acx 3
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc3     xchg    al,ch           ;                       RES D,3
                RES_Acx 3
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec3     xchg    al,cl           ;                       RES E,3
                RES_Acx 3
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc3     xchg    al,dh           ;                       RES H,3
                RES_Acx 3
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc3     xchg    al,dl           ;                       RES L,3
                RES_Acx 3
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac3     RES_Acx 3               ;                       RES A,3
                jmp     [red_what_ret]

ZCB_RES_bHLbc3  add     word [red_z80_clk_bus],1 ;                   RES (HL),3
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc3 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 3
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 3
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac3   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 3
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc4     xchg    al,bh           ;                       RES B,4
                RES_Acx 4
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc4     xchg    al,bl           ;                       RES C,4
                RES_Acx 4
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc4     xchg    al,ch           ;                       RES D,4
                RES_Acx 4
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec4     xchg    al,cl           ;                       RES E,4
                RES_Acx 4
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc4     xchg    al,dh           ;                       RES H,4
                RES_Acx 4
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc4     xchg    al,dl           ;                       RES L,4
                RES_Acx 4
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac4     RES_Acx 4               ;                       RES A,4
                jmp     [red_what_ret]

ZCB_RES_bHLbc4  add     word [red_z80_clk_bus],1 ;                   RES (HL),4
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc4 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 4
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 4
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac4   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 4
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc5     xchg    al,bh           ;                       RES B,5
                RES_Acx 5
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc5     xchg    al,bl           ;                       RES C,5
                RES_Acx 5
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc5     xchg    al,ch           ;                       RES D,5
                RES_Acx 5
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec5     xchg    al,cl           ;                       RES E,5
                RES_Acx 5
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc5     xchg    al,dh           ;                       RES H,5
                RES_Acx 5
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc5     xchg    al,dl           ;                       RES L,5
                RES_Acx 5
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac5     RES_Acx 5               ;                       RES A,5
                jmp     [red_what_ret]

ZCB_RES_bHLbc5  add     word [red_z80_clk_bus],1 ;                   RES (HL),5
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc5 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 5
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 5
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac5   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 5
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc6     xchg    al,bh           ;                       RES B,6
                RES_Acx 6
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc6     xchg    al,bl           ;                       RES C,6
                RES_Acx 6
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc6     xchg    al,ch           ;                       RES D,6
                RES_Acx 6
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec6     xchg    al,cl           ;                       RES E,6
                RES_Acx 6
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc6     xchg    al,dh           ;                       RES H,6
                RES_Acx 6
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc6     xchg    al,dl           ;                       RES L,6
                RES_Acx 6
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac6     RES_Acx 6               ;                       RES A,6
                jmp     [red_what_ret]

ZCB_RES_bHLbc6  add     word [red_z80_clk_bus],1 ;                   RES (HL),6
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc6 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 6
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 6
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac6   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 6
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_RES_Bc7     xchg    al,bh           ;                       RES B,7
                RES_Acx 7
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_RES_Cc7     xchg    al,bl           ;                       RES C,7
                RES_Acx 7
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_RES_Dc7     xchg    al,ch           ;                       RES D,7
                RES_Acx 7
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_RES_Ec7     xchg    al,cl           ;                       RES E,7
                RES_Acx 7
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_RES_Hc7     xchg    al,dh           ;                       RES H,7
                RES_Acx 7
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_RES_Lc7     xchg    al,dl           ;                       RES L,7
                RES_Acx 7
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_RES_Ac7     RES_Acx 7               ;                       RES A,7
                jmp     [red_what_ret]

ZCB_RES_bHLbc7  add     word [red_z80_clk_bus],1 ;                   RES (HL),7
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_bHLbc7 sub     word [red_z80_clk_bus],2 ;                  RES (HL+d),7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_RES_Bc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),B,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 7
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Cc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),C,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                RES_Acx 7
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Dc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),D,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ec7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),E,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Hc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXh,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_RES_Lc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IXl,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Hc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYh,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_RES_Lc7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),IYl,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_RES_Ac7   sub     word [red_z80_clk_bus],2 ;                   RES (HL+d),A,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                RES_Acx 7
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc0     xchg    al,bh           ;                       SET B,0
                SET_Acx 0
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc0     xchg    al,bl           ;                       SET C,0
                SET_Acx 0
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc0     xchg    al,ch           ;                       SET D,0
                SET_Acx 0
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec0     xchg    al,cl           ;                       SET E,0
                SET_Acx 0
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc0     xchg    al,dh           ;                       SET H,0
                SET_Acx 0
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc0     xchg    al,dl           ;                       SET L,0
                SET_Acx 0
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac0     SET_Acx 0               ;                       SET A,0
                jmp     [red_what_ret]

ZCB_SET_bHLbc0  add     word [red_z80_clk_bus],1 ;                   SET (HL),0
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc0 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 0
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 0
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac0   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 0
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc1     xchg    al,bh           ;                       SET B,1
                SET_Acx 1
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc1     xchg    al,bl           ;                       SET C,1
                SET_Acx 1
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc1     xchg    al,ch           ;                       SET D,1
                SET_Acx 1
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec1     xchg    al,cl           ;                       SET E,1
                SET_Acx 1
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc1     xchg    al,dh           ;                       SET H,1
                SET_Acx 1
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc1     xchg    al,dl           ;                       SET L,1
                SET_Acx 1
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac1     SET_Acx 1               ;                       SET A,1
                jmp     [red_what_ret]

ZCB_SET_bHLbc1  add     word [red_z80_clk_bus],1 ;                   SET (HL),1
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc1 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 1
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 1
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac1   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 1
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc2     xchg    al,bh           ;                       SET B,2
                SET_Acx 2
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc2     xchg    al,bl           ;                       SET C,2
                SET_Acx 2
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc2     xchg    al,ch           ;                       SET D,2
                SET_Acx 2
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec2     xchg    al,cl           ;                       SET E,2
                SET_Acx 2
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc2     xchg    al,dh           ;                       SET H,2
                SET_Acx 2
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc2     xchg    al,dl           ;                       SET L,2
                SET_Acx 2
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac2     SET_Acx 2               ;                       SET A,2
                jmp     [red_what_ret]

ZCB_SET_bHLbc2  add     word [red_z80_clk_bus],1 ;                   SET (HL),2
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc2 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 2
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 2
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac2   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 2
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc3     xchg    al,bh           ;                       SET B,3
                SET_Acx 3
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc3     xchg    al,bl           ;                       SET C,3
                SET_Acx 3
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc3     xchg    al,ch           ;                       SET D,3
                SET_Acx 3
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec3     xchg    al,cl           ;                       SET E,3
                SET_Acx 3
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc3     xchg    al,dh           ;                       SET H,3
                SET_Acx 3
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc3     xchg    al,dl           ;                       SET L,3
                SET_Acx 3
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac3     SET_Acx 3               ;                       SET A,3
                jmp     [red_what_ret]

ZCB_SET_bHLbc3  add     word [red_z80_clk_bus],1 ;                   SET (HL),3
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc3 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 3
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 3
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac3   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 3
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc4     xchg    al,bh           ;                       SET B,4
                SET_Acx 4
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc4     xchg    al,bl           ;                       SET C,4
                SET_Acx 4
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc4     xchg    al,ch           ;                       SET D,4
                SET_Acx 4
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec4     xchg    al,cl           ;                       SET E,4
                SET_Acx 4
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc4     xchg    al,dh           ;                       SET H,4
                SET_Acx 4
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc4     xchg    al,dl           ;                       SET L,4
                SET_Acx 4
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac4     SET_Acx 4               ;                       SET A,4
                jmp     [red_what_ret]

ZCB_SET_bHLbc4  add     word [red_z80_clk_bus],1 ;                   SET (HL),4
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc4 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 4
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 4
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac4   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 4
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc5     xchg    al,bh           ;                       SET B,5
                SET_Acx 5
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc5     xchg    al,bl           ;                       SET C,5
                SET_Acx 5
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc5     xchg    al,ch           ;                       SET D,5
                SET_Acx 5
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec5     xchg    al,cl           ;                       SET E,5
                SET_Acx 5
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc5     xchg    al,dh           ;                       SET H,5
                SET_Acx 5
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc5     xchg    al,dl           ;                       SET L,5
                SET_Acx 5
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac5     SET_Acx 5               ;                       SET A,5
                jmp     [red_what_ret]

ZCB_SET_bHLbc5  add     word [red_z80_clk_bus],1 ;                   SET (HL),5
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc5 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 5
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 5
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac5   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 5
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc6     xchg    al,bh           ;                       SET B,6
                SET_Acx 6
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc6     xchg    al,bl           ;                       SET C,6
                SET_Acx 6
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc6     xchg    al,ch           ;                       SET D,6
                SET_Acx 6
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec6     xchg    al,cl           ;                       SET E,6
                SET_Acx 6
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc6     xchg    al,dh           ;                       SET H,6
                SET_Acx 6
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc6     xchg    al,dl           ;                       SET L,6
                SET_Acx 6
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac6     SET_Acx 6               ;                       SET A,6
                jmp     [red_what_ret]

ZCB_SET_bHLbc6  add     word [red_z80_clk_bus],1 ;                   SET (HL),6
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc6 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 6
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 6
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac6   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 6
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_SET_Bc7     xchg    al,bh           ;                       SET B,7
                SET_Acx 7
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_SET_Cc7     xchg    al,bl           ;                       SET C,7
                SET_Acx 7
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_SET_Dc7     xchg    al,ch           ;                       SET D,7
                SET_Acx 7
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_SET_Ec7     xchg    al,cl           ;                       SET E,7
                SET_Acx 7
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_SET_Hc7     xchg    al,dh           ;                       SET H,7
                SET_Acx 7
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_SET_Lc7     xchg    al,dl           ;                       SET L,7
                SET_Acx 7
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_SET_Ac7     SET_Acx 7               ;                       SET A,7
                jmp     [red_what_ret]

ZCB_SET_bHLbc7  add     word [red_z80_clk_bus],1 ;                   SET (HL),7
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_bHLbc7 sub     word [red_z80_clk_bus],2 ;                  SET (HL+d),7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_SET_Bc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),B,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 7
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Cc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),C,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                SET_Acx 7
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Dc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),D,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ec7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),E,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Hc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXh,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_SET_Lc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IXl,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Hc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYh,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_SET_Lc7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),IYl,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_SET_Ac7   sub     word [red_z80_clk_bus],2 ;                   SET (HL+d),A,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                SET_Acx 7
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
                mov     [red_xx_32_store],edx
                offset_mem_for_memwr_HLd
                call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc0     xchg    al,bh           ;                       BIT B,0
                BIT_Acx 0
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc0     xchg    al,bl           ;                       BIT C,0
                BIT_Acx 0
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc0     xchg    al,ch           ;                       BIT D,0
                BIT_Acx 0
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec0     xchg    al,cl           ;                       BIT E,0
                BIT_Acx 0
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc0     xchg    al,dh           ;                       BIT H,0
                BIT_Acx 0
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc0     xchg    al,dl           ;                       BIT L,0
                BIT_Acx 0
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac0     BIT_Acx 0               ;                       BIT A,0
                jmp     [red_what_ret]

ZCB_BIT_bHLbc0  add     word [red_z80_clk_bus],1 ;                   BIT (HL),0
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di      ;  NB - don't want to write to mem
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc0:                       ;                       BIT (HL+d),0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc0:                          ;                       BIT (HL+d),B,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 0
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc0:                          ;                       BIT (HL+d),C,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 0
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc0:                              ;                   BIT (HL+d),D,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec0:                              ;                   BIT (HL+d),E,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc0:                              ;                   BIT (HL+d),IXh,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc0:                              ;                   BIT (HL+d),IXl,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc0:                              ;                   BIT (HL+d),IYh,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc0:                              ;                   BIT (HL+d),IYl,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac0:                              ;                   BIT (HL+d),A,0
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 0
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc1     xchg    al,bh           ;                       BIT B,1
                BIT_Acx 1
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc1     xchg    al,bl           ;                       BIT C,1
                BIT_Acx 1
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc1     xchg    al,ch           ;                       BIT D,1
                BIT_Acx 1
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec1     xchg    al,cl           ;                       BIT E,1
                BIT_Acx 1
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc1     xchg    al,dh           ;                       BIT H,1
                BIT_Acx 1
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc1     xchg    al,dl           ;                       BIT L,1
                BIT_Acx 1
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac1     BIT_Acx 1               ;                       BIT A,1
                jmp     [red_what_ret]

ZCB_BIT_bHLbc1  add     word [red_z80_clk_bus],1 ;                   BIT (HL),1
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc1:                            ;                  BIT (HL+d),1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc1:                              ;                   BIT (HL+d),B,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 1
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc1:                              ;                   BIT (HL+d),C,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 1
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc1:                              ;                   BIT (HL+d),D,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec1:                              ;                   BIT (HL+d),E,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc1:                              ;                   BIT (HL+d),IXh,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc1:                              ;                   BIT (HL+d),IXl,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc1:                              ;                   BIT (HL+d),IYh,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc1:                              ;                   BIT (HL+d),IYl,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac1:                              ;                   BIT (HL+d),A,1
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 1
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc2     xchg    al,bh           ;                       BIT B,2
                BIT_Acx 2
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc2     xchg    al,bl           ;                       BIT C,2
                BIT_Acx 2
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc2     xchg    al,ch           ;                       BIT D,2
                BIT_Acx 2
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec2     xchg    al,cl           ;                       BIT E,2
                BIT_Acx 2
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc2     xchg    al,dh           ;                       BIT H,2
                BIT_Acx 2
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc2     xchg    al,dl           ;                       BIT L,2
                BIT_Acx 2
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac2     BIT_Acx 2               ;                       BIT A,2
                jmp     [red_what_ret]

ZCB_BIT_bHLbc2  add     word [red_z80_clk_bus],1 ;                   BIT (HL),2
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc2:                            ;                  BIT (HL+d),2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc2:                              ;                   BIT (HL+d),B,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 2
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc2:                              ;                   BIT (HL+d),C,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 2
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc2:                              ;                   BIT (HL+d),D,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec2:                              ;                   BIT (HL+d),E,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc2:                              ;                   BIT (HL+d),IXh,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc2:                              ;                   BIT (HL+d),IXl,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc2:                              ;                   BIT (HL+d),IYh,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc2:                              ;                   BIT (HL+d),IYl,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac2:                              ;                   BIT (HL+d),A,2
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 2
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc3     xchg    al,bh           ;                       BIT B,3
                BIT_Acx 3
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc3     xchg    al,bl           ;                       BIT C,3
                BIT_Acx 3
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc3     xchg    al,ch           ;                       BIT D,3
                BIT_Acx 3
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec3     xchg    al,cl           ;                       BIT E,3
                BIT_Acx 3
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc3     xchg    al,dh           ;                       BIT H,3
                BIT_Acx 3
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc3     xchg    al,dl           ;                       BIT L,3
                BIT_Acx 3
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac3     BIT_Acx 3               ;                       BIT A,3
                jmp     [red_what_ret]

ZCB_BIT_bHLbc3  add     word [red_z80_clk_bus],1 ;                   BIT (HL),3
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc3:                            ;                  BIT (HL+d),3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc3:                              ;                   BIT (HL+d),B,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 3
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc3:                              ;                   BIT (HL+d),C,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 3
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc3:                              ;                   BIT (HL+d),D,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec3:                              ;                   BIT (HL+d),E,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc3:                              ;                   BIT (HL+d),IXh,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc3:                              ;                   BIT (HL+d),IXl,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc3:                              ;                   BIT (HL+d),IYh,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc3:                              ;                   BIT (HL+d),IYl,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac3:                              ;                   BIT (HL+d),A,3
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 3
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc4     xchg    al,bh           ;                       BIT B,4
                BIT_Acx 4
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc4     xchg    al,bl           ;                       BIT C,4
                BIT_Acx 4
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc4     xchg    al,ch           ;                       BIT D,4
                BIT_Acx 4
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec4     xchg    al,cl           ;                       BIT E,4
                BIT_Acx 4
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc4     xchg    al,dh           ;                       BIT H,4
                BIT_Acx 4
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc4     xchg    al,dl           ;                       BIT L,4
                BIT_Acx 4
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac4     BIT_Acx 4               ;                       BIT A,4
                jmp     [red_what_ret]

ZCB_BIT_bHLbc4  add     word [red_z80_clk_bus],1 ;                   BIT (HL),4
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc4:                            ;                  BIT (HL+d),4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc4:                              ;                   BIT (HL+d),B,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 4
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc4:                              ;                   BIT (HL+d),C,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 4
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc4:                              ;                   BIT (HL+d),D,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec4:                              ;                   BIT (HL+d),E,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc4:                              ;                   BIT (HL+d),IXh,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc4:                              ;                   BIT (HL+d),IXl,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc4:                              ;                   BIT (HL+d),IYh,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc4:                              ;                   BIT (HL+d),IYl,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac4:                              ;                   BIT (HL+d),A,4
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 4
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc5     xchg    al,bh           ;                       BIT B,5
                BIT_Acx 5
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc5     xchg    al,bl           ;                       BIT C,5
                BIT_Acx 5
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc5     xchg    al,ch           ;                       BIT D,5
                BIT_Acx 5
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec5     xchg    al,cl           ;                       BIT E,5
                BIT_Acx 5
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc5     xchg    al,dh           ;                       BIT H,5
                BIT_Acx 5
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc5     xchg    al,dl           ;                       BIT L,5
                BIT_Acx 5
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac5     BIT_Acx 5               ;                       BIT A,5
                jmp     [red_what_ret]

ZCB_BIT_bHLbc5  add     word [red_z80_clk_bus],1 ;                   BIT (HL),5
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc5:                            ;                  BIT (HL+d),5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc5:                              ;                   BIT (HL+d),B,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 5
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc5:                              ;                   BIT (HL+d),C,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 5
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc5:                              ;                   BIT (HL+d),D,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec5:                              ;                   BIT (HL+d),E,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc5:                              ;                   BIT (HL+d),IXh,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc5:                              ;                   BIT (HL+d),IXl,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc5:                              ;                   BIT (HL+d),IYh,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc5:                              ;                   BIT (HL+d),IYl,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac5:                              ;                   BIT (HL+d),A,5
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 5
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc6     xchg    al,bh           ;                       BIT B,6
                BIT_Acx 6
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc6     xchg    al,bl           ;                       BIT C,6
                BIT_Acx 6
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc6     xchg    al,ch           ;                       BIT D,6
                BIT_Acx 6
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec6     xchg    al,cl           ;                       BIT E,6
                BIT_Acx 6
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc6     xchg    al,dh           ;                       BIT H,6
                BIT_Acx 6
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc6     xchg    al,dl           ;                       BIT L,6
                BIT_Acx 6
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac6     BIT_Acx 6               ;                       BIT A,6
                jmp     [red_what_ret]

ZCB_BIT_bHLbc6  add     word [red_z80_clk_bus],1 ;                   BIT (HL),6
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc6:                            ;                  BIT (HL+d),6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc6:                              ;                   BIT (HL+d),B,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 6
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc6:                              ;                   BIT (HL+d),C,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 6
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc6:                              ;                   BIT (HL+d),D,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec6:                              ;                   BIT (HL+d),E,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc6:                              ;                   BIT (HL+d),IXh,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc6:                              ;                   BIT (HL+d),IXl,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc6:                              ;                   BIT (HL+d),IYh,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc6:                              ;                   BIT (HL+d),IYl,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac6:                              ;                   BIT (HL+d),A,6
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 6
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZCB_BIT_Bc7     xchg    al,bh           ;                       BIT B,7
                BIT_Acx 7
                xchg    al,bh
                jmp     [red_what_ret]
ZCB_BIT_Cc7     xchg    al,bl           ;                       BIT C,7
                BIT_Acx 7
                xchg    al,bl
                jmp     [red_what_ret]
ZCB_BIT_Dc7     xchg    al,ch           ;                       BIT D,7
                BIT_Acx 7
                xchg    al,ch
                jmp     [red_what_ret]
ZCB_BIT_Ec7     xchg    al,cl           ;                       BIT E,7
                BIT_Acx 7
                xchg    al,cl
                jmp     [red_what_ret]
ZCB_BIT_Hc7     xchg    al,dh           ;                       BIT H,7
                BIT_Acx 7
                xchg    al,dh
                jmp     [red_what_ret]
ZCB_BIT_Lc7     xchg    al,dl           ;                       BIT L,7
                BIT_Acx 7
                xchg    al,dl
                jmp     [red_what_ret]
ZCB_BIT_Ac7     BIT_Acx 7               ;                       BIT A,7
                jmp     [red_what_ret]

ZCB_BIT_bHLbc7  add     word [red_z80_clk_bus],1 ;                   BIT (HL),7
                mov     edi,edx
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_bHLbc7:                            ;                  BIT (HL+d),7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]

ZxDCB_BIT_Bc7:                              ;                   BIT (HL+d),B,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 7
                mov     bh,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Cc7:                              ;                   BIT (HL+d),C,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ecx
                mov     ecx,edi
                xchg    al,cl
                BITdAcx 7
                mov     bl,al
                xchg    al,cl
                mov     edi,ecx
                mov     ecx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Dc7:                              ;                   BIT (HL+d),D,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                mov     ch,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ec7:                              ;                   BIT (HL+d),E,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                mov     cl,al
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Hc7:                              ;                   BIT (HL+d),IXh,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                xchg    edx,esi
                mov     dh,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZDDCB_BIT_Lc7:                              ;                   BIT (HL+d),IXl,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                xchg    edx,esi
                mov     dl,al
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Hc7:                              ;                   BIT (HL+d),IYh,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                xchg    edx,esi
                ror     edx,0x010
                mov     dh,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZFDCB_BIT_Lc7:                              ;                   BIT (HL+d),IYl,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                xchg    edx,esi
                ror     edx,0x010
                mov     dl,al
                rol     edx,0x010
                xchg    edx,esi
                xchg    al,bl
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]
ZxDCB_BIT_Ac7:                              ;                   BIT (HL+d),A,7
                mov     edi,edx
                offset_di_for_memrd_HLd
                call    mem_rd_di
                mov     [red_temp_32_store],ebx
                mov     ebx,edi
                xchg    al,bl
                BITdAcx 7
                mov     bl,al
                mov     edi,ebx
                mov     ebx,[red_temp_32_store]
;               call    mem_wr_di
                jmp     [red_what_ret]



;***********************************************************************;
;***********************************************************************;
;                                                                       ;
; Reference section.                                                    ;
;                                                                       ;
; Source: http://www.geocities.com/SiliconValley/Peaks/z80undoc3.txt    ;
;                                                                       ;
;***********************************************************************;
;***********************************************************************;

; 
; 		Z80 Undocumented Features (in software behaviour)
; 		=================================================
; 
; 			By Sean Young (sean@msxnet.org)
; 			  Version 0.3 October 1998
; 
; 
; This document describes all I know about undocumented features of the Z80.
; Almost everything is taken from other people's pages, so don't think this
; all my own stuff. However, I think there is some new stuff here too; about 
; interrupts and flag affection and some other stuff. 
; 
; All information has a references to where I got it from. It's marked with
; [number], which refers to the references at the end of this document (5.3).
; For example, [5] indicates I've figured it out myself by experimenting.
; 
; As you've probably noticed, I'm no (true) native English speaker. Any 
; comments, additions, about technical stuff or language is very welcome.
; 
; You can always find the latest version of this document at:
; 
; 	http://www.msxnet.org/tech/
; 
; 
; 1) Undocumented Opcodes
;  1.1) Prefixes in general
;  1.2) CB Prefix
;  1.3) DD Prefix
;  1.4) FD Prefix
;  1.5) ED Prefix
;  1.6) DDCB Prefix
;  1.7) FDCB Prefix
;  1.8) Combinations of prefixes
; 
; 2) Undocumented Flag affection & operation
;  2.1) Flag register bits
;  2.2) All instructions that affect the flags
;   2.2.1) 8 Bit arthmetic and logical
;   2.2.2) 16 Bit arithmetic
;   2.2.3) BIT instruction
;   2.2.4) Other non-block instructions
;   2.2.5) Memory block instructions
;   2.2.6) I/O block instructions
;  2.3) Undocumented operation
;   2.3.1) DAA
;   2.3.2) I/O Instructions
;   2.3.3) Block instructions
; 
; 3) Interrupts and I register
;  3.1) Non-maskable Interrupts (NMI)
;  3.2) Maskable Interrupts (INT)
;  3.4) Things affecting the IFFs
;  3.5) HALT instruction
;  3.6) Where can interrupts occur?
; 
; 4) Timing and R register
;  4.1) R register and memory refresh
;  4.2) Instruction timings
;  
; 5) Other information
;  5.1) Power on defaults
;  5.2) Errors in official documentation
;  5.3) References and other resources
; 
; 
; 1) Undocumented Opcodes [1]
; ---------------------------
; 
; There are quite a few undocumented opcodes/instructions to the Z80. This
; section should describe every possible opcode (so you know what
; instruction will be executed, whatever the combination of values is).
; 
; Check out my Z80 Opcodes list for a complete list of all instructions.
; 
; 	http://www.msxnet.org/tech/Z80/z80opcod.txt
; 
; 
; 1.1) Prefixes in General [1]
; 
; There are the following prefixes: CBh, EDh. DDh. FDh. DDCBh and FDCBh. 
; Prefixes change the way the following opcodes are interpreted. 
; 
; All instructions without a prefix (without anything like the above before
; it) are documented in the official documentation.
; 
; 
; 1.2) CB Prefix [1]
; 
; An opcode with a CBh prefix is a rotate, shift or bit test/set/reset 
; instruction. There are a few instructions missing from the official list, 
; which are usually denoted with SLL (Shift Logical Left). It works like 
; SLA, for one exception: it sets bit 0 (SLA resets it).
; 
; 	CB30	SLL B
; 	CB31	SLL C
; 	CB32	SLL D
; 	CB33	SLL E
; 	CB34	SLL H
; 	CB35	SLL L
; 	CB36	SLL (HL)
; 	CB37	SLL A
; 
; 
; 1.3) DD Prefix [1]
; 
; In general, after a DD prefix the instruction is executed as if the DD
; weren't there. There are some exceptions:
; 
; * Any access to HL is treated as an access to IX (except EX DE,HL and EXX
;   and the ED prefixed instructions that use HL).
; * Any access to (HL) is changed to (IX+d), where d is a signed displacement
;   byte placed after the main opcode (except JP (HL), which isn't indirect
;   anyways.
; * Any access to H is treated as an access to IXh (the high byte of IX)
;   Except if (IX+d) is accessed.
; * Any access to L is treated as an access to IXl (the low byte of IX)
;   Except if (IX+d) is accessed.
; * A DD prefix before a CB selects a completely different instruction 
;   set (see 1.5).
; 
; So for some examples:
; 
; Without DD prefix	With DD prefix
; LD H,(HL)		LD H,(IX+d)
; LD H,A			LD IXh,A
; LD L,H			LD IXl,IXh
; JP (HL)			JP (IX)
; LD DE,0			LD DE,0
; LD HL,0			LD IX,0
; 
; Note LD IXl,IYh is not possible: only IX or IY is accessed in one instruction.
; 
; 
; 1.3) FD Prefix [1]
; 
; This prefix has the same effect as the DD prefix, though IY is used in
; stead of IX.
; 
; 
; 1.4) ED Prefix [1]
; 
; There are a number of undocumented EDxx instructions, of which most are
; duplicates of documented instructions. Any instruction not listed has
; no effect (just like 2 NOP instructions).
; 
; The complete list except for the block instructions: (* = undocumented)
; 
;         ED40   IN B,(C)                 ED60   IN H,(C)
;         ED41   OUT (C),B                ED61   OUT (C),H
;         ED42   SBC HL,BC                ED62   SBC HL,HL
;         ED43   LD (nn),BC               ED63   LD (nn),HL
;         ED44   NEG                      ED64 * NEG
;         ED45   RETN                     ED65 * RETN
;         ED46   IM 0                     ED66 * IM 0
;         ED47   LD I,A                   ED67   RRD
;         ED48   IN C,(C)                 ED68   IN L,(C)
;         ED49   OUT (C),C                ED69   OUT (C),L
;         ED4A   ADC HL,BC                ED6A   ADC HL,HL
;         ED4B   LD BC,(nn)               ED6B   LD HL,(nn)
;         ED4C * NEG                      ED6C * NEG
;         ED4D   RETI                     ED6D * RETN
;         ED4E * IM 0                     ED6E * IM 0
;         ED4F   LD R,A                   ED6F   RLD
; 
;         ED50   IN D,(C)                 ED70 * IN (C) / IN F,(C)
;         ED51   OUT (C),D                ED71 * OUT (C),0
;         ED52   SBC HL,DE                ED72   SBC HL,SP
;         ED53   LD (nn),DE               ED73   LD (nn),SP
;         ED54 * NEG                      ED74 * NEG
;         ED55 * RETN                     ED75 * RETN
;         ED56   IM 1                     ED76 * IM 1
;         ED57   LD A,I                   ED77 * NOP
;         ED58   IN E,(C)                 ED78   IN A,(C)
;         ED59   OUT (C),E                ED79   OUT (C),A
;         ED5A   ADC HL,DE                ED7A   ADC HL,SP
;         ED5B   LD DE,(nn)               ED7B   LD SP,(nn)
;         ED5C * NEG                      ED7C * NEG
;         ED5D * RETN                     ED7D * RETN
;         ED5E   IM 2                     ED7E * IM 2
;         ED5F   LD A,R                   ED7F * NOP
; 
; The ED70 instruction reads from I/O port C, but does not store the result. 
; It just affects the flags like the other IN x,(C) instruction. ED71 simply
; outs the value 0 to I/O port C.
; 
; The ED63 is a duplicate of the 22 instruction (LD (nn),HL) just like the
; ED6B is a duplicate of the 2A instruction. Of course the timings are
; different. These instructions are listed in the official documentation.
; 
; According to Gerton Lunter (gerton@math.rug.nl):
;     The instructions ED 4E and ED 6E are IM 0 equivalents: when FF was put
;     on the bus (physically) at interrupt time, the Spectrum continued to
;     execute normally, whereas when an EF (RST #28) was put on the bus it
;     crashed, just as it does in that case when the Z80 is in the official
;     interrupt mode 0.  In IM 1 the Z80 just executes a RST #38 (opcode FF)
;     no matter what is on the bus.
; 
; [5] All the RETI/RETN instructions are the same, all like the RETN 
; instruction. So they all, including RETI, copy IFF2 to IFF1. More information
; on RETI and RETN and IM x is in the part about Interrupts and I register (3).
; 
; 
; 1.5) DDCB Prefixes [1][5]
; 
; The undocumented DDCB instructions store the result (if any) of the 
; operation in one of the seven all-purpose registers, which one depends on
; the lower 3 bits of the last byte of the opcode (not operand, so not
; the offset).
; 
;         000     B
;         001     C
;         010     D
;         011     E
;         100     H
;         101     L
;         110     (none: documented opcode)
;         111     A
; 
; The documented DDCB0106 is RLC (IX+01h). So, clear the lower three bits 
; (DDCB0100) and something is done to register B. The result of the RLC
; (which is stored in (IX+01h)) is now also stored in register B. Effectively,
; it does the following:
; 
; 	LD B,(IX+01h)
; 	RLC B
; 	LD (IX+01h),B
; 
; So you get double value for money. The result is stored in B and (IX+01h). The
; most common notation is: RLC (IX+01h),B
; 
; I've once seen this notation:
; 
; 	RLC (IX+01h)
; 	LD B,(IX+01h)
; 
; That's not correct: B contains the rotated value, even if (IX+01h) points to 
; ROM memory.
; 
; The DDCB SET and RES instructions do the same thing as the shift/rotate
; instructions:
; 
; DDCB10C0	SET 0,(IX+10h),B
; DDCB10C1	SET 0,(IX+10h),C
; DDCB10C2	SET 0,(IX+10h),D
; DDCB10C3	SET 0,(IX+10h),E
; DDCB10C4	SET 0,(IX+10h),H
; DDCB10C5	SET 0,(IX+10h),L
; DDCB10C6	SET 0,(IX+10h)
; DDCB10C7	SET 0,(IX+10h),A
; 
; So the value of (IX+10h) with bit 0 set, is also stored in register A.
; 
; The DDCB BIT instructions do not store any value; the merely test a bit.
; That's why the undocumented DDCB BIT instructions are no different from
; the official ones:
; 
; DDCB d 78       BIT 7,(IX+d)
; DDCB d 79       BIT 7,(IX+d)
; DDCB d 7A       BIT 7,(IX+d)
; DDCB d 7B       BIT 7,(IX+d)
; DDCB d 7C       BIT 7,(IX+d)
; DDCB d 7D       BIT 7,(IX+d)
; DDCB d 7E       BIT 7,(IX+d) - official one
; DDCB d 7F       BIT 7,(IX+d)
; 
; 
; 1.6) FDCB Prefixes [1]
; 
; Same as for the DDCB prefix, though IY is used in stead of IX.
; 
; 
; 1.7) Combinations of Prefixes [5]
; 
; This part may be of some interest to emulator coders. Here we define
; what happens if strange sequences of prefixes appear in the instruction
; cycle of the Z80.
; 
; In general, DD and FD can change the following instruction a bit, (use IX or
; IY in stead of HL), and ED and CB select a completely different set of 
; instructions.
; 
; EDxx: Any DD or FD prefix before it is ignored, no matter how many. A CBh
; prefix can't appear before it because CBED is interpreted as SET 5,L; 
; a separate instruction. If the second byte of an EDxx opcode is CB, DD, FD
; or ED, it has no effect on following instructions:
; 
; 	EDFD210000	NOP; LD HL,0
; 
; FDxx/DDxx: Any DD or FD before the FDxx/DDxx is ignored; in a sequence of DDs 
; and FDs, it is only the last one that counts. The ones before just act like 
; NOPs. CB before an DD or FD is not possible (as a prefix) because CBDD and
; CBFD are instructions in themselves, so is EDFD and EDDD. 
; 
; CBxx: DD and FD prefixes are possible, with the effect as stated above. ED
; isn't possible; EDCB is an instruction in itself (which doesn't do anything).
; 
; 
; 2) Undocumented flag affection & operation [5][3]
; -------------------------------------------------
; 
; 2.1) Flag register bits [5][3]
; 
; The F (flag) register has the following bits:
; 
; F	7  |  6  |   5  |  4  |  3  |  2  |  1  |  0
; -----------+-----+------+-----+-----+-----+-----+-----
; flag	S  |  Z  |   5  |  H  |  3  | P/V |  N  |  C
; 
; S flag: 
;   set if the 2-complement value is negative. It's simply a copy of the most 
;   significant bit.
; 
; Z flag:
;   Set if the value is zero.
; 
; 5 flag:
;   A copy of bit 5 of the result.
; 
; H flag:
;   The half-carry of an addition/subtraction (from bit 3 to 4). Needed for
;   DAA correction.
; 
; 3 flag:
;   A copy of bit 3 of the result.
; 
; P/V flag:
;   This flag can either be the parity of the result, or the 2-compliment
;   signed overflow (2-compliment value doesn't fit in the register).
; 
; N flag:
;   Shows whether the last operation was an addition (0) or an subtraction (1).
;   This information is needed for DAA.
; 
; C flag:
;   The carry flag, set if there was a carry after the most significant bit.
; 
; In this part, the following notation is used for flag affection:
; 
; * indicates the effect is non-standard (see the notes).
; 0 indicates the flag is reset.
; 1 indicates the flag is set.
; - indicates the flag is not affected.
; S,Z,5,H,3,P,V,N,C indicate the flag is set as above.
; 
; Notice there's no ``unknown'' or ``undefined'' here. :)
; 
; 
; 2.2) All instructions that affect the flags [1][2][3]
; 
; This section lists all the instructions that affect the flags, and how they
; do that. 
; 
; 
; 2.2.1) 8 Bit arithmetic and logical
; 
; The 8 bit arithmetic group is straightforward:
; 
; ADD/ADC/SUB/SBC       SZ5H3VNC  
; 
; All standard flags ..
; 
; CP r                  SZ*H*VNC
; 
; A CP is simply a SUB with the result thrown away. Flag 5 and 3 are copied 
; from the operand, not the result (which is thrown away anyways).
; 
; INC/DEC r             SZ5H3VN-
; 
; AND r                 SZ513P00
; OR/XOR r              SZ503P00
; 
; RLCA/RLA/RRCA/RRA     --503-0C
; RLC/RL/RRC/RR r       SZ503P0C
; SLA/SLL/SRA/SRL r     SZ503P0C  SLL is like SLA except bit 0 gets set
; RRD/RLD               SZ503P0-  Flags set on result in A
; 
; 
; 2.2.2) 16 Bit arithmetic
; 
; The 16 bit additions are a bit more complicated. 16 bit additions are done in 
; two stages: first the lower bytes are added, then the two higher bytes. The 
; S,5,H,3 flags are affected as by the second 8 bit addition. Z is set if the 
; whole 16 bit result is 0.
; 
; ADD s                 --***-0C
; ADC/SBC s             SZ***VNC
; 
; 
; 2.2.3) BIT instruction [5] [6]
; 
; BIT n,r               *Z513*0-	r is one of the 8 bit registers.
; 
; BIT behaves much AND r,2^n with the result throw away and the C flag is 
; unaffected. So in effect: 
; Z = set if the tested bit is reset. 
; S = set if n=7 and tested bit is set.
; 5 = set if n=5 and tested bit is set.
; 3 = set if n=3 and tested bit is set.
; P/V = set like Z.
; 
; BIT n,(IX/IY+d)	      *Z*1**0-
; 
; Again, P/V is set like Z and S is if set n=7 and the tested bit is set. But
; the 5 and 3 flags are different: they are a copy of bit 5 and 3 of the high
; byte of IX/IY+d (the value after the addition of d to IX or IY).
; 
; BIT n,(HL)            *Z*1**0-
; 
; P/V, Z, S set as the other BIT instructions, but the 5 and 3 flags are
; different. Okay, brace yourself. They are a copy of an internal register of
; the Z80, which is set as follows:
; 
; ADD HL,xx      (use high byte of HL, i.e. H, before the addition)
; LD r,(IX/IY+d) (use high byte of the resulting address IX/IY+d)
; JR d           (use high byte target address of the jump)
; LD r,r'	       (not affected)
; others         not tested -- any additions welcome!
; 
; 
; 2.2.4) Other non-block instructions [2] [5]
; 
; CCF                   --***-0*
; 
; C=1-C, H as old C. 5, 3 from A register
; 
; SCF                   --*0*-01
; CPL                   --*1*-1-
; 
; 5, 3 from A register.
; 
; NEG                   SZ5H3V1C
; 
; DAA                   SZ5*3P-*
; 
; See below (2.3) for more information.
; 
; LD A,R/LD A,I         SZ503*0-
; 
; P/V contains a copy of IFF2.
; 
; IN r,(C)              SZ503P0-
; 
; Also true for IN F,(C) / IN (C). 
; 
; 
; 2.2.5) Memory block instructions [2] [5]
; 
; LDI/LDIR/LDD/LDDR     --*0**0-  P/V set if BC not 0
;                                 5 is bit 1 of (transferred byte + A)
;                                 3 is bit 3 of (transferred byte + A)
; 
; So add A to the (last) transferred byte (from (HL)), and bit 1 of that
; 8 bit value is flag 5, and bit 3 is flag 3.
; 
; CPI/CPIR/CPD/CPDR     SZ*H**1-  P/V set if BC not 0
;                                 S,Z,H from (A - (HL) ) as in CP (HL)
;                                 3 is bit 3 of (A - (HL) - H)
;                                 5 is bit 1 of (A - (HL) - H)
; 
; CPI instructions are weird too. The test is simply like a CP (HL). Flag 3 and
; 5 are set like this: Take A, subtract the last (HL), and then decrease it
; with 1 if the H flag was set (/after/ the CP). Bit 1 of this value is flag
; 5, bit 3 is flag 3.
; 
; 
; 2.2.6) I/O Block instructions [5] [6]
; 
; INI/INIR/IND/INDR     SZ5*3***  Flags affected as in DEC B
; OUTI/OTIR/OUTD/OTDR   SZ5*3***  Flags affected as in DEC B
; 
; Another weird one. S,Z,5,3 are affected by the decrease of B, like in DEC B.
; The N flag is a copy of bit 7 of the last value read from/written too the
; I/O port. The C and H flag is set as follows: Take register C, add one to it
; if the instruction increases HL otherwise decrease it by one. Now, add the
; the value of the I/O port (read or written) to it, and the carry of this
; last addition is copied to the C and H flag (so C and H flag are the same). 
; Beats me, but tests show it to be true. 
; 
; Pedro Gimeno (pgimeno@geocities.com) has figured out how the P/V flag is
; affected. The result depends on the B register, the lower 3 bits of the C
; register and the lower 3 bits of the value from the I/O port. This next part 
; is written by him. Whatever he says about INI/INIR is true for OUTI/OTIR,
; and what he says about IND/INDR is true for OUTD/OTDR.
; 
; I use the notation C.2 to mean bit 2 of C. "inp" is the byte being
; input.
; 
; First, look at bits 1 and 0 of both C and inp and look up this table to
; obtain a temporary result:
; 
; C.1   C.0  inp.1 inp.0   Temp1
;  0     0     0     0       0
;  0     0     0     1       0
;  0     0     1     0       1
;  0     0     1     1       0
;  0     1     0     0       0
;  0     1     0     1       1
;  0     1     1     0       0
;  0     1     1     1       1
;  1     0     0     0       1
;  1     0     0     1       0
;  1     0     1     0       1
;  1     0     1     1       1
;  1     1     0     0       0
;  1     1     0     1       1
;  1     1     1     0       1
;  1     1     1     1       0
; 
; You'll need also an additional temporary result which depends on B (all
; of its bits), according to the following pseudocode:
; 
; If B.3 = B.2 = B.1 = B.0 = 0 then [i.e. if B and 0Fh = 0 then]
;   let Temp2 = Parity(B) xor (B.4 or (B.6 and not B.5))
; else
;   let Temp2 = Parity(B) xor (B.0 or (B.2 and not B.1))
; 
; where Parity(B) is defined as not (B.0 xor B.1 xor B.2 xor B.3 xor B.4
; xor B.5 xor B.6 xor B.7), i.e. 1 if B has an even number of bits = 1,
; and 0 otherwise (the usual parity function).
; 
; Note that I obtained the operation (B.0 or (B.2 and not B.1)) from the
; next table using Karnaugh maps:
; 
; B.2 through B.0  000 001 010 011 100 101 110 111
; Value             0   1   0   1   1   1   0   1
; 
; For completeness here's the full table of values of Temp2 according to
; B:
; 
;     0123456789ABCDEF <- (low nibble)
; 00  0011010011001011
; 10  0100101100110100
; 20  1100101100110100
; 30  1011010011001011
; 40  0100101100110100
; 50  1011010011001011
; 60  0011010011001011
; 70  0100101100110100
; 80  1100101100110100
; 90  1011010011001011
; A0  0011010011001011
; B0  0100101100110100
; C0  1011010011001011
; D0  0100101100110100
; E0  1100101100110100
; F0  1011010011001011
; ^
; (high nibble)
; 
; And finally, the value of the P/V flag is:
; 
; P/V = Temp1 xor Temp2 xor C.2 xor inp.2
; IND / INDR:
; 
; The operations to obtain the P/V flag are the same as for INI/INIR but
; the table used to lookup Temp1 is different:
; 
; C.1   C.0  inp.1 inp.0   Temp1
;  0     0     0     0       0
;  0     0     0     1       1
;  0     0     1     0       0
;  0     0     1     1       0
;  0     1     0     0       1
;  0     1     0     1       0
;  0     1     1     0       0
;  0     1     1     1       1
;  1     0     0     0       0
;  1     0     0     1       0
;  1     0     1     0       1
;  1     0     1     1       0
;  1     1     0     0       0
;  1     1     0     1       1
;  1     1     1     0       0
;  1     1     1     1       1
; 
; 
; 2.3) Undocumented operation [1][5]
; 
; 
; 2.3.1) DAA [4] [5]
; 
; This instruction is useful when you're using BCD values. After an addition
; or subtraction, DAA makes the correction so the value is the correct BCD
; value again.
; 
; The correction done is as in the following table [4]:
; 
; N | C | high   | H | low    | # added | C after
;   |   | nibble |   | nibble | to A    | execution
; --+---+--------+---+--------+---------+-----------
; add, adc, inc operands:
; 0   0   0-9      0   0-9      00        0
;     0   0-8      0   a-f      06        0
;     0   0-9      1   0-3      06        0
;     0   a-f      0   0-9      60        1
;     0   9-f      0   a-f      66        1
;     0   a-f      1   0-3      66        1
;     1   0-2      0   0-9      60        1
;     1   0-2      0   a-f      66        1
;     1   0-3      1   0-3      66        1
; -------------------------------------------------
; sub, sbc, dec, neg operands:
; 1   0   0-9      0   0-9      00        0
;     0   0-8      1   6-f      fa        0
;     1   7-f      0   0-9      a0        1
;     1   6-f      1   6-f      9a        1
; 
; This table is all I know about DAA operation. 
; 
; Emulator builders: this instruction is hard to emulate. The Intel 80x86
; equivalents are more or less the same, but differ for some input values
; (not in the table?). If you're not using assembly, the best way IMHO is
; to put all possible output values (for A and F registers) in a table. The
; output depends on the following inputs: A register and H,N,C flags. That's
; 11 bits in total, so the table will have size: 2^11*2 = 4096 bytes, which
; isn't too bad.
; 
; 
; 2.3.2) I/O Instructions
; 
; The I/O instructions use the whole of the address bus, not just the lower
; 8 bits. So in fact, you can have 65536 I/O ports in a Z80 system (the Spectrum
; uses this). IN r,(C), OUT (C),r and all the I/O block instructions put the
; whole of BC on the address bus. IN A,(n) and OUT (n),A put A*256+n on the 
; address bus. 
; 
; 
; 2.3.3) Block instructions
; 
; The repeated block instructions simply decrease the PC by two so the 
; instruction is simply re-executed. So interrupts can occur during block 
; instructions. So, LDIR is simply LDI + if BC is not 0, decrease PC by 2.
; 
; 
; 3) Interrupts and I register [4]
; --------------------------------
; 
; For the interrupts, the following things are important: Interrupt Mode
; (set with the IM instructions), the interrupt flip-flops (IFF1 and IFF2),
; and the I register.
; 
; There are two types of interrupts, maskable and non-maskable. The maskable
; type is ignored if IFF1 is reset. Non-maskable interrupts (NMI) will
; always occur, and they have a higher priority, so if the two are requested 
; at the same time the NMI will occur.
; 
; 
; 3.1) Non-maskable interrupts (NMI) [4]
; 
; When a NMI is accepted, IFF1 is reset. At the end of the routine, IFF1 must
; be restored (so the running program is not affected). That's why IFF2 is
; there; to keep a copy of IFF1.
; 
; An NMI is accepted when the NMI pin on the Z80 is made low. The Z80 responds 
; to the /change/ of the line from +5 to 0. When this happens, a call is done 
; to address 0066h and IFF1 is reset so the routine isn't bothered by maskable 
; interrupts. The routine should end with an RETN (RETurn from Nmi) which is 
; just a usual RET, but also copies IFF2 to IFF1, so the IFFs are the same as 
; before the interrupt.
; 
; You can check whether interrupts were disabled or not during an NMI by using 
; the LD A,I or LD A,R instruction. These instructions copy IFF2 to the P/V 
; flag.
; 
; 
; 3.2) Maskable interrupts (INT) [4]
; 
; If the INT line is 0 volt and IFF1 is set, a maskable interrupt is accepted - 
; whether or not the the last INT routine has finished. That's why you should 
; never enable interrupts during such a routine, and make sure that the device 
; that generated it has put the INT line up to +5 volt again before ending the 
; routine.
; 
; When an INT is accepted, both IFFs are cleared, preventing another interrupt 
; from occurring which would end up as an infinite loop. What happens next 
; depends on the Interrupt Mode.
; 
; IM 0:
;   The instruction on the bus is executed (usually an RST instruction, but
;   it can be anything else -- even instructions larger than one byte). 
;   The I register is ignored.
; 
; IM 1:
;   An RST 38h is executed, no matter what value is put on the
;   bus or what value the I register has.
; 
; IM 2:
;   A call is made to the address read from memory. What address is
;   read from is calculated as follows: (I register) * 256 + (value on bus). 
;   Of course a word (two bytes) are read, comprising an address where the
;   call is made to. In this way, you can have a vector table for interrupts.
; 
; At the end of a maskable interrupt, the interrupts should be enabled again. 
; You can assume that was the state of the IFFs because otherwise the interrupt 
; wasn't accepted. (snappy huh). So, an INT routine always ends with an EI and 
; a RET (RETI according to the official documentation,
; more about that later):
; 
; INT:	.
; 	.
; 	.
; 	EI
; 	RETI (or RET)
; 
; Note a fact about EI: a maskable interrupt isn't accepted directly after it,
; so the next opportunity for an INT is after the RETI. This is very useful;
; if the INT is still low, an INT is generated again. If this happens a lot and
; the interrupt is generated before the RETI, the stack could overflow (since
; the routine is called again and again). But this property of EI prevents this.
; 
; DI is not necessary at the start of the interrupt routine: the Z80 disables 
; them when accepting the interrupt.
; 
; You can use RET in stead of RETI too, it depends on hardware setup. RETI
; is only useful if you have something like a Z80 PIO to support daisy-chaining:
; queueing interrupts. The PIO can detect that the routine has ended by the
; opcode of RETI, and let another device generate an interrupt. That is why
; I called all the undocumented EDxx RET instructions RETN: All of them 
; operate like RETN, the only difference to RETI is its specific opcode. 
; (Which the Z80 PIO recognises.)
; 
; 
; 3.3) Things affecting the IFFs [4]
; 
; All the IFF related things are:
; 
; 		IFF1	IFF2
; /CPU reset	0	0
; DI		0	0
; EI		1	1
; Accept INT	0	0
; Accept NMI	0	-
; RETI/N		IFF2	-	All the EDxx RETI/N instructions
; LD A,I/LD A,R	-	-	Copies IFF2 into P/V flag
; 
; Note the fact about RETI/N: all the EDxx RET instructions copy IFF2 into
; IFF1, even RETI! The other one-byte RET commands (RET, RET NZ etc.)
; don't do this.
; 
; Since every INT routine must end with this sequence:
; 
; 	EI
; 	RETI (or RET, but RETI officially)
; 
; It does not matter that RETI copies IFF2 into IFF1; both are set anyways.
; 
; [5] There seems to be some dispute about whether or not when an NMI is 
; accepted, IFF1 is copied into IFF2 before clearing IFF1. Some official docs 
; even states this, but I've been experimenting with nested NMIs (if it were
; correct, the IFF states would be lost after a nested NMI) and I've found
; it to be wrong. Only IFF1 is reset, that's all.
; 
; If you're working with a Z80 system without NMIs (like the MSX), you can 
; forget all about the two separate IFFs; since a NMI isn't ever generated, 
; the two will always be the same. 
; 
; 
; 3.5) HALT instruction [4]
; 
; The HALT instruction halts the Z80; it starts executing NOPs, until a maskable 
; or non-maskable interrupt is accepted. Only then does the Z80 increase the PC 
; and continues with the next instruction. During the HALT state, the HALT line 
; is set. The PC is increased before the interrupt routine is called.
; 
; 
; 3.6) Where can interrupts occur? [5]
; 
; During execution of instructions, interrupts won't be accepted. Only /between/ 
; instructions. This is also true for prefixed instructions.
; 
; Directly after an EI or DI instruction, interrupts aren't accepted. They're
; accepted again after the instruction after the EI (RET in this example). So for 
; example, look at this MSX2 routine that reads a scanline from the keyboard:
; 
; 	LD	C,A
; 	DI
; 	IN	A,(0AAh)
; 	AND 	0F0h
; 	ADD	A,C
; 	OUT	(0AAh),A
; 	EI
; 	IN	A,(0A9h)
; 	RET
; 
; You can assume that there never is an interrupt after the EI, before the
; IN A,(0A9h) -- which would be a problem because the MSX interrupt routine
; reads the keyboard too.
; 
; Using this feature of EI, it is possible to check whether it is true that
; interrupts are never issued during instructions:
; 
; 	DI
; 	<make sure INT is active>
; 	EI
; 	<insert instruction to test>
; 
; INT:
; 	<store PC where INT was issued>
; 	RET
; 
; And yes, for all instructions, including the prefixed ones, interrupts are
; never issued during an instruction. Only after the tested instruction. 
; Remember that block instructions simply re-execute themselves (by decreasing
; the PC with 2) so an interrupt is accepted after each iteration.
; 
; Another predictable test is this: at the <insert instruction to test> insert
; a large sequence of EI instructions. Of course, during execution of the EI
; instructions, no interrupts are accepted. 
; 
; But now for the interesting stuff. EDED or CBCB are just instructions in
; themselves, so interrupts are issued after them. But DD and FD are just
; prefixes, which only slightly affects the next opcode. If you test a large
; sequence of DDs or FDs, the same happens as with the EI instruction: no
; interrupts are accepted during the execution of these sequences.
; 
; This makes sense, if you think of DD and FD as a prefix which set the
; "use IX instead of HL" or "use IY instead of HL" flag. If an interrupt was
; issued after DD or FD, this flag information would be lost, and:
; 
; 	DD 21 00 00 	LD IX,0
; 
; could be interpreted as a simple LD HL,0, if the interrupt was after the last
; DD. Which never happens, so the implementation is correct.
; 
; Because of this,
; 
; 	DD DD 21 00 00
; 
; is also always executed as LD IX,0 and never as LD HL,0.
; 
; For the same reason, NMIs aren't accepted after a DD or FD too (I haven't
; tested this but I can't imagine otherwise). The Z80 docs say that no 
; interrupts are accepted after EI or DI, which also includes NMIs (also not
; tested).
; 
; 
; 4) Timing and R register [1]
; ----------------------------
; 
; 4.1) R register and memory refresh [1][4]
; 
; During every first machine cycle (beginning of an instruction or part
; of it -- prefixes have their own M1 two), the memory refresh cycle is
; issued. The whole of IR is put on the address bus, and the _RFSH is lowered.
; It unclear whether the Z80 increases the R register before or after putting
; IR on the bus. 
; 
; The R register is increased at every first machine cycle (M1). Bit 7 of
; the register is never changed by this; only the lower 7 bits are included
; in the addition. So bit 7 stays the same, but it can be changed using the
; LD R,A instruction.
; 
; Instructions without a prefix increase R by one. Instructions with an ED, CB,
; DD, FD prefix, increase R by two, and so do the DDCBxxxx and FDCBxxxx 
; instructions (weird enough). Just a stray DD or FD increases the R by
; one. LD A,R and LD R,A access the R register after it is increased (by 
; the instruction itself). 
; 
; Remember that the block instructions simply decrease the PC with two, so
; the instructions are re-executed. So LDIR increased R by BC times 2 (note that
; in the case of BC = 0, R is increased by 10000h times 2, effectively 0).
; 
; Accepting an maskable or non-maskable interrupt increases the R by one.
; 
; After a hardware reset, or after power on, the R register is reset to 0.
; 
; That should cover all there is to say about the R register. It is often
; used in programs for a random value, which is good but of course not
; truly random.
; 
; 
; 4.2) Instruction timings [1][4]
; 
; Not much stuff here, as I'm no cycle expert. The opcode list does show
; the timings of all instructions though:
; 
; 	http://www.msxnet.org/tech/Z80/z80opcod.txt
; 
; Undocumented timings:
; 
; * Stray DD or FD: same as a NOP (increases R by one).
; 
; * EDxx (not one of the list above or block instruction): 
;   Same as two NOPs. Increases R by 2.
; 
; * Accepting interrupts (all one M1 and thus increase R by one): (T states)
; 
;   IM 0: 13 (assuming the instruction is a RST)
;   IM 1: 13
;   IM 2: 19
;   NMI : 11
; 
; 
; 5) Other Information
; --------------------
; 
; 5.1) Power on defaults [4]
; 
; After a reset (a hard reset of course) or power on, all registers are set
; to 0, the IFFs are reset and the interrupt mode is 0. So, since the PC is
; 0, the Z80 starts executing code at address 0.
; 
; 
; 5.2) Errors in official documentation [5]
; 
; In different official (?) documentation, I've seen some errors. Some don't
; have all of these mistakes, so your documentation may not be flawed but 
; these are just things to look out for.
; 
; * The Flag affection summary table shows that LDI/LDIR/LDD/LDDR instructions 
;   leave the S and Z in an undefined state. This is not correct; the S and Z
;   flags are unaffected (like the same documentation says).
; 
; * Similary, the same table shows that CPI/CPIR/CPD/CPDR leave the S and H
;   flags in an undefined state. Not true, they are affected as defined 
;   elsewhere in the documentation.
; 
; * Also, the table says about INI/OUTD/etc "Z=0 if B <> 0 otherwise Z=0";
;   of course the latter should be Z=1.
; 
; * The INI/INIR/IND/INDR/OUTI/OUTD/OTIR/OTDR instructions do affect the C
;   flag (some official documentation says they leave it unaffected, 
;   important!) and the N flag isn't always set but may also be reset (see 
;   above for exact operation).
; 
; * When an NMI is accepted, the IFF1 isn't copied to IFF2. Only IFF1
;   is reset.
; 
; * In the 8-bit Load Group, the last two bits of the second byte of
;   the LD r,(IX + d) opcode should be 10 and not 01.
; 
; * In the 16-bit Arithmetic Group, bit 6 of the second byte of the
;   ADD IX, pp opcode should be 0, not 1.
; 
; * IN x,(C) resets the H flag, it never sets it. Some documentation states
;   it is set according to the result of the operation; bullocks, no
;   arithmetic is done in this instruction.
; 
; 
; 5.3) References and other resources
; 
; Here are a few locations on the web, with information about the Z80. 
; 
; The references in this document are marked here two. They are in no particular
; order, and maybe I made a mistake somewhere too.. So include the Standard
; Disclaimer.
; 
; http://www.acorn.co.uk/~mrison/en/cpc/tech.html [3]
; 	Mark Rison Z80 page for !CPC.
; 
; ftp://ftp.ping.de/pub/misc/emulators/yaze-1.10.tar.gz [2]
; 	YAZE (Yet Another Z80 Emulator). This is a CPM emulator by Frank 
; 	Cringle. It emulates almost every undocumented flag, very good
; 	emulator. Also includes a very good instruction exerciser.
; 
; http://www.geocities.com/SiliconValley/Peaks/3938/z80_home.htm
; 	Z80 Family Official Support Page by Thomas Scherrer. Very good, a lot
; 	information there.
; 
; http://www.kendalls.demon.co.uk/cssfaq/tech_z80.html
; 	Spectrum FAQ technical information.
; 
; ftp://ftp.void.jump.org/pub/sinclair/emulators/pc/dos/z80-400.zip [1]
; 	Gerton Lunter's Spectrum emulator (Z80). In the package there is
; 	a file TECHINFO.DOC, which contains a lot of interesting 
; 	information.
; 
; http://users.aol.com/autismuk/emu.html
; 	A TRS80 (Z80 based) emulator in assembly.
; 
; http://www.zilog.com/ 
; 	Guess what, the makers of the little bastard. Not much information
; 	here (you can order technical books though).
; 
; http://www.msxnet.org/tech/ [5]
; 	My own (Sean Young) Z80 page. You can find this document there
; 	and some other stuff.
; 
; Mostek Z80 Programming Manual [4]
; 	A /very/ good reference book to the Z80.
; 
; Pedro Gimeno (pgimeno@geocities.com) [6]







