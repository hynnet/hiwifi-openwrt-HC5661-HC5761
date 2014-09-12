

#ifndef NOR_H
#define NOR_H


/* describe the interface to the SD combo core using a C structure:
 *
 * NB Use only 32 bit accessess. i.e. read using data element decode using bit
 *    mapped structure.
 */

struct SD_combo {
	/* u32 register offset 0 : CONFIG. */
	union {
		struct {
			unsigned int bank_sel :3;
			unsigned :5;
			unsigned int fifo_auto_reset :1;
			unsigned int fifo_auto_flush :1;
			unsigned int reserv1 :6;
			unsigned int slot_0 : 1;
			unsigned int slot_1 : 1;
			unsigned int reserv2 : 14 ;
		} cfg;
		u32 data;
	} config;
	/* u32 register offset 4 : COMMAND */
	union {
		u32 data;
		struct {
			unsigned int start :1;
			unsigned int reserve1 :1;
			unsigned int aa_enable :1;
			unsigned int imm_cmd :1;
			unsigned int timeout_enable :1;
			unsigned int multiblock :1;
			unsigned int stream :1;
			unsigned int reserve2 :1;
			unsigned int length :3;
			unsigned int reserve3 :5;
			unsigned int response_type :3;
			unsigned int response_ignore :1;
			unsigned int reserve4 :1;
			unsigned int command_type :3;
			unsigned int command_index :6;
			unsigned int command_prefix :2;
		} cmd;
	} command ;
	/* u32 register offset 8 : ARGUMENT */
	u32 argument;
	/* u32 register offset C : DATA SIZE */
	union {
		u32 data;
		struct {
			unsigned length :11;
			unsigned  :5;
			unsigned number :9;
		} field;
	} data_size;
	/* u32 register offset 10 : COMMAND EXTENSION */
	u32 cmd_exnt;
	/* u32 register offset 14 : EXPECTED RESPONSE */
	u32 exp_reply;
	/* u32 register offset 18 : RESPONSE MASK */
	u32 reply_mask;
	/* u32 register offset 1C : COMMAND STATUS */
	union {
		u32 data;
		struct {
			unsigned cmd_fin      :1;
			unsigned data_fin     :1;
			unsigned cmd_pending  :1;
			unsigned aa_pending   :1;
			unsigned trans_active :1;
			unsigned trans_error  :1;
			unsigned :1;
			unsigned respns_timeout :1;
			unsigned cmd_resp_idle :1;
			unsigned cmd_resp_busy :1;
			unsigned cmd_resp_err  :1;
			unsigned cmd_resp_silent :1;
			unsigned data_idle :1;
			unsigned data_busy :1;
			unsigned data_error :1;
			unsigned card_busy :1;
			unsigned sdio_intrpt :1;
		} field;
	} cmd_status;
	/* u32 register offset 20 : COMMAND ERROR STATUS */
	union {
		u32 data;
		struct {
			unsigned rsp_framing :1;
			unsigned rsp_crc :1;
			unsigned rsp_index :1;
			unsigned rsp_arg :1;
			unsigned :4;
			unsigned unexpected:1;
			unsigned :7;
			unsigned aa_rsp_frame :1;
			unsigned aa_rsp_crc :1;
			unsigned aa_rsp_index :1;
			unsigned aa_resp_arg :1;
		} field;
	} cmd_error;
	/* u32 register offset 24 : RESPONSE QUAD 1 */
	u32 response1;
	/* u32 register offset 28 : RESPONSE QUAD 2 */
	u32 response2;
	/* u32 register offset 2C : RESPONSE 1 AUTO ABORT */
	u32 response_aa1;
	/* u32 register offset 30 : RESPONSE 2 AUTO ABORT */
	u32 response_aa2;
	/* u32 register offset 34 : DATA ERROR STATUS */
	union {
		u32 data;
		struct {
			unsigned write_inv_crc :1;
			unsigned write_no_crc  :1;
			unsigned :1;
			unsigned write_crc_status :5;
			unsigned read_inv_crc :1;
			unsigned read_inv_frame :1;
			unsigned read_incomplete :1;
			unsigned :1;
			unsigned read_timeout :1;
			unsigned write_timeout :1;
			unsigned busy_timeout :1;
			unsigned :1;
			unsigned unexpected_data :1;
		} field;
	} data_error;
	/* u32 register offset 38 : BLOCK COUNT */
	u32 block_count;
	/* u32 register offset 3C : LINE STATE */
	u32 line_state;
	/* u32 register offset 40 : FIFO PORT */
	u32 fifo_port;
	/* u32 register offset 44 : FIFO CONTROL */
	u32 fifo_control;
	/* u32 register offset 48 : FIFO STATUS */
	union {
		u32 data;
		struct {
			u16 fill_level;
			unsigned fifo_realy_empty :1;
			unsigned fifo_empty :1;
			unsigned fifo_nearly_empty :1;
			unsigned :1;
			unsigned fifo_full:1;
			unsigned fifo_nearly_full :1;
			unsigned :6;
			unsigned fifo_underrun:1;
			unsigned fifo_invalid_cpu_access :1;
			unsigned fifo_invalid_phy_access:1;
		} field;
	} fifo_status;
	/* u32 register offset 4C : INTERRUPT MASK */ 
	u32 interrupt_mask;
	/* u32 register offset 50 : CLOCK CONTROL */
	u32 clock_control;
	/* u32 register offset 54 : GAP CONTROL */
	u32 gap_control;
	/* u32 register offset 58 : TIMEOUT CONTROL */
	u32 timeout_ctrol;
	/* u32 register offset 5C : SM_CONTROL */
	u32 sm_control;
	/* u32 register offset 60.. : BANK CONFIG */
	union {
		u32 data;
		struct {
			unsigned if_mode :3;
			unsigned clock_stop_en :1;
			unsigned sdio_irq_sup :1;
			unsigned sdio_irq_mod :1;
			unsigned sdio_irq_enable :1;
			unsigned :1;
			unsigned sdio_read_wait :1;
			unsigned signal_1 :1;
			unsigned signal_2 :1;
			unsigned :1;
			unsigned  gap_length :3;
			unsigned out_edge:1;
			unsigned cmd_slot:3;
			unsigned dat0_slot :3;
			unsigned dat1_slot :3;
			unsigned dat3_slot :3;
			unsigned dat4_slot :3;
		} field;
	} bank_config[2];
};


#define NOR_BLOCK     512

struct load_data {
	u32 *buffer;
	u32 start;
	u32 length;
	u32 result;
};

extern void init_NOR(void);
extern void read_NOR(struct load_data * data);

extern void close_NOR(void);


#endif
