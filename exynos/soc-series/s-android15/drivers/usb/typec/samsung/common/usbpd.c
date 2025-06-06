/*
*	USB PD Driver - Protocol Layer
*/

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/completion.h>

#if IS_ENABLED(CONFIG_IFCONN_NOTIFIER)
#include <linux/usb/typec/samsung/common/usbpd.h>
#include <linux/misc/samsung/ifconn/ifconn_notifier.h>

extern struct pdic_notifier_data pd_noti;
#else
#include <linux/usb/typec/slsi/common/usbpd.h>
#include <linux/pm_wakeup.h>
#include <linux/version.h>
#include <linux/battery/battery_notifier.h>

extern struct pdic_notifier_struct pd_noti;
#endif

#define MS_TO_NS(msec)		((msec) * 1000 * 1000)

void usbpd_wake_lock(struct wakeup_source *ws)
{
	__pm_stay_awake(ws);
}

void usbpd_wake_unlock(struct wakeup_source *ws)
{
	__pm_relax(ws);
}

int usbpd_set_wake_lock(struct usbpd_data *pd_data)
{
	struct wakeup_source *usbpd_ws = NULL;

	usbpd_ws = wakeup_source_register(NULL, "pdic_wake");

	if (usbpd_ws == NULL) {
		pr_info("%s, Fail to register ws\n", __func__);
		goto err;
	}

	pd_data->usbpd_ws = usbpd_ws;

	return 0;
err:
	return -1;
}

void usbpd_timer_vdm_start(struct usbpd_data *pd_data)
{
	ktime_get_ts64(&pd_data->time_vdm);
}
EXPORT_SYMBOL(usbpd_timer_vdm_start);

long long usbpd_check_timer_vdm(struct usbpd_data *pd_data)
{
	uint32_t ms = 0;
	uint32_t sec = 0;
	struct timespec64 time;

	ktime_get_ts64(&time);

    sec = time.tv_sec - pd_data->time_vdm.tv_sec;
    ms = (time.tv_nsec - pd_data->time_vdm.tv_nsec) / 1000000;

    return (sec * (long long)1000) + ms;
}
EXPORT_SYMBOL(usbpd_check_timer_vdm);

void usbpd_timer1_start(struct usbpd_data *pd_data)
{
	ktime_get_ts64(&pd_data->time1);
}

int usbpd_check_time1(struct usbpd_data *pd_data)
{
	uint32_t ms = 0;
	uint32_t sec = 0;
	struct timespec64 time;

	ktime_get_ts64(&time);

	sec = time.tv_sec - pd_data->time1.tv_sec;
	ms = (time.tv_nsec - pd_data->time1.tv_nsec) / 1000000;

	pd_data->check_time.tv_sec = time.tv_sec;
	pd_data->check_time.tv_nsec = time.tv_nsec;

	/* for demon update after boot */
	if ((sec > 100)) {
		pr_info("%s, timer changed\n", __func__);
		usbpd_timer1_start(pd_data);
		return 0;
	}

	return (sec * 1000) + ms;
}

void usbpd_timer2_start(struct usbpd_data *pd_data)
{
	ktime_get_ts64(&pd_data->time2);
}

int usbpd_check_time2(struct usbpd_data *pd_data)
{
	int ms = 0;
	int sec = 0;
	struct timespec64 time;

	ktime_get_ts64(&time);

	sec = time.tv_sec - pd_data->time2.tv_sec;
	ms = (time.tv_nsec - pd_data->time2.tv_nsec) / 1000000;

	return (sec * 1000) + ms;
}

static void increase_message_id_counter(struct usbpd_data *pd_data)
{
	pd_data->counter.message_id_counter++;
	pd_data->counter.message_id_counter %= 8;
/*
	if (pd_data->counter.message_id_counter++ > USBPD_nMessageIDCount)
		pd_data->counter.message_id_counter = 0;
*/
}

static void rx_layer_init(struct protocol_data *rx)
{
	int i;

	rx->stored_message_id = USBPD_nMessageIDCount+1;
	rx->msg_header.word = 0;
	rx->state = 0;
	rx->status = DEFAULT_PROTOCOL_NONE;
	for (i = 0; i < USBPD_MAX_COUNT_MSG_OBJECT; i++)
		rx->data_obj[i].object = 0;
}

static void tx_layer_init(struct protocol_data *tx)
{
	int i;

	tx->stored_message_id = USBPD_nMessageIDCount+1;
	tx->msg_header.word = 0;
	tx->state = 0;
	tx->status = DEFAULT_PROTOCOL_NONE;
	for (i = 0; i < USBPD_MAX_COUNT_MSG_OBJECT; i++)
		tx->data_obj[i].object = 0;
}

static void tx_discard_message(struct protocol_data *tx)
{
	int i;

	tx->msg_header.word = 0;
	for (i = 0; i < USBPD_MAX_COUNT_MSG_OBJECT; i++)
		tx->data_obj[i].object = 0;
}

void usbpd_init_protocol(struct usbpd_data *pd_data)
{
	rx_layer_init(&pd_data->protocol_rx);
	tx_layer_init(&pd_data->protocol_tx);
	pd_data->msg_id = USBPD_nMessageIDCount + 1;
}

void usbpd_init_counters(struct usbpd_data *pd_data)
{
	pr_info("%s: init counter\n", __func__);
	pd_data->counter.retry_counter = 0;
	pd_data->counter.message_id_counter = 0;
	pd_data->counter.caps_counter = 0;
#if 0
	pd_data->counter.hard_reset_counter = 0;
#endif
	pd_data->counter.swap_hard_reset_counter = 0;
	pd_data->counter.discover_identity_counter = 0;
}

void usbpd_policy_reset(struct usbpd_data *pd_data, unsigned flag)
{
	if (flag == HARDRESET_RECEIVED) {
		pd_data->policy.rx_hardreset = 1;
		dev_info(pd_data->dev, "%s Hard reset\n", __func__);
		usbpd_manager_remove_new_cap(pd_data);
	} else if (flag == SOFTRESET_RECEIVED) {
		pd_data->policy.rx_softreset = 1;
		dev_info(pd_data->dev, "%s Soft reset\n", __func__);
	} else if (flag == PLUG_EVENT) {
		if (!pd_data->policy.plug_valid)
			pd_data->policy.plug = 1;
		pd_data->policy.plug_valid = 1;
		dev_info(pd_data->dev, "%s ATTACHED\n", __func__);
	} else if (flag == PLUG_DETACHED) {
		pd_data->policy.plug_valid = 0;
		dev_info(pd_data->dev, "%s DETACHED\n", __func__);
		pd_data->counter.hard_reset_counter = 0;
		pd_data->source_get_sink_obj.object = 0;
		usbpd_manager_remove_new_cap(pd_data);
	}
}

protocol_state usbpd_protocol_tx_phy_layer_reset(struct protocol_data *tx)
{
	return PRL_Tx_Wait_for_Message_Request;
}

protocol_state usbpd_protocol_tx_wait_for_message_request(struct protocol_data
								*tx)
{
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);
	protocol_state state = PRL_Tx_Wait_for_Message_Request;

	/* S2MM004 PDIC already retry.
	if (pd_data->counter.retry_counter > USBPD_nRetryCount) {
		pd_data->counter.retry_counter = 0;
		return state;
	}
	*/
	if (pd_data->counter.retry_counter > 0) {
		pd_data->counter.retry_counter = 0;
		return state;
	}

	pd_data->counter.retry_counter = 0;

	if (!tx->msg_header.word)
		return state;

	if (tx->msg_header.num_data_objs == 0 &&
			tx->msg_header.msg_type == USBPD_Soft_Reset)
		state = PRL_Tx_Layer_Reset_for_Transmit;
	else
		state = PRL_Tx_Construct_Message;

	return state;
}

protocol_state usbpd_protocol_tx_layer_reset_for_transmit(struct protocol_data *tx)
{
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);

	dev_info(pd_data->dev, "%s\n", __func__);

	pd_data->counter.message_id_counter = 0;
	pd_data->protocol_rx.state = PRL_Rx_Wait_for_PHY_Message;

	/* TODO: check Layer Reset Complete */
	return PRL_Tx_Construct_Message;
}

protocol_state usbpd_protocol_tx_construct_message(struct protocol_data *tx)
{
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);

	tx->msg_header.msg_id = pd_data->counter.message_id_counter;
	tx->status = DEFAULT_PROTOCOL_NONE;

	if (PDIC_OPS_PARAM2_FUNC(tx_msg, pd_data, &tx->msg_header, tx->data_obj)) {
		dev_err(pd_data->dev, "%s error\n", __func__);
		return PRL_Tx_Construct_Message;
	}
	return PRL_Tx_Wait_for_PHY_Response;
}

protocol_state usbpd_protocol_tx_wait_for_phy_response(struct protocol_data *tx)
{
#if 0
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);
	protocol_state state = PRL_Tx_Wait_for_PHY_Response;
	u8 CrcCheck_cnt = 0;

	/* wait to get goodcrc */
	/* mdelay(1); */

	/* polling */
	/* pd_data->phy_ops.poll_status(pd_data); */

	for (CrcCheck_cnt = 0; CrcCheck_cnt < 2; CrcCheck_cnt++) {
		if (pd_data->phy_ops.get_status(pd_data, MSG_GOODCRC)) {
			pr_info("%s : %p\n", __func__, pd_data);
			state = PRL_Tx_Message_Sent;
			dev_info(pd_data->dev, "got GoodCRC.\n");
			return state;
		}

		if (!CrcCheck_cnt)
			pd_data->phy_ops.poll_status(pd_data); /* polling */
	}

	return PRL_Tx_Check_RetryCounter;
#endif
	return PRL_Tx_Message_Sent;
}

protocol_state usbpd_protocol_tx_match_messageid(struct protocol_data *tx)
{
	/* We don't use this function.
	   S2MM004 PDIC already check message id for incoming GoodCRC.

	struct usbpd_data *pd_data = protocol_tx_to_usbpd(protocol_tx);
	protocol_state state = PRL_Tx_Match_MessageID;

	dev_info(pd_data->dev, "%s\n",__func__);

	if (pd_data->protocol_rx.msg_header.msg_id
			== pd_data->counter.message_id_counter)
		state = PRL_Tx_Message_Sent;
	else
		state = PRL_Tx_Check_RetryCounter;

	return state;
	*/
	return PRL_Tx_Message_Sent;
}

protocol_state usbpd_protocol_tx_message_sent(struct protocol_data *tx)
{
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);

	increase_message_id_counter(pd_data);
	tx->status = MESSAGE_SENT;
	/* clear protocol header buffer */
	tx->msg_header.word = 0;

	return PRL_Tx_Wait_for_Message_Request;
}

protocol_state usbpd_protocol_tx_check_retrycounter(struct protocol_data *tx)
{
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);

	/* S2MM004 PDIC already do retry.
	   Driver SW doesn't do retry.

	if (++pd_data->counter.retry_counter > USBPD_nRetryCount) {
		state = PRL_Tx_Transmission_Error;
	} else {
		state = PRL_Tx_Construct_Message;
	}

	return PRL_Tx_Check_RetryCounter;
	*/
	++pd_data->counter.retry_counter;
	return PRL_Tx_Transmission_Error;
}

protocol_state usbpd_protocol_tx_transmission_error(struct protocol_data *tx)
{
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);

	dev_err(pd_data->dev, "%s\n", __func__);

	increase_message_id_counter(pd_data);
	tx->status = TRANSMISSION_ERROR;

	return PRL_Tx_Wait_for_Message_Request;
}

protocol_state usbpd_protocol_tx_discard_message(struct protocol_data *tx)
{
	/* This state is for Only Ping message */
	struct usbpd_data *pd_data = protocol_tx_to_usbpd(tx);

	dev_err(pd_data->dev, "%s\n", __func__);
	tx_discard_message(tx);
	increase_message_id_counter(pd_data);

	return PRL_Tx_PHY_Layer_Reset;
}

void usbpd_set_ops(struct device *dev, usbpd_phy_ops_type *ops)
{
	struct usbpd_data *pd_data = (struct usbpd_data *) dev_get_drvdata(dev);


	pd_data->phy_ops.soft_reset				= ops->soft_reset;
	pd_data->phy_ops.driver_reset			= ops->driver_reset;
	pd_data->phy_ops.set_lpm_mode			= ops->set_lpm_mode;
	pd_data->phy_ops.set_normal_mode		= ops->set_normal_mode;
	pd_data->phy_ops.pd_vbus_short_check	= ops->pd_vbus_short_check;
	pd_data->phy_ops.usbpd_authentic		= ops->usbpd_authentic;
	pd_data->phy_ops.send_pd_info			= ops->send_pd_info;
	pd_data->phy_ops.pr_swap				= ops->pr_swap;
	pd_data->phy_ops.control_option_command	= ops->control_option_command;
	pd_data->phy_ops.set_pcp_clk			= ops->set_pcp_clk;
	pd_data->phy_ops.get_vbus_short_check	= ops->get_vbus_short_check;

	pd_data->phy_ops.poll_status			= ops->poll_status;
	pd_data->phy_ops.get_status				= ops->get_status;
	pd_data->phy_ops.hard_reset				= ops->hard_reset;
	pd_data->phy_ops.op_mode_clear			= ops->op_mode_clear;
	pd_data->phy_ops.get_lpm_mode			= ops->get_lpm_mode;
	pd_data->phy_ops.get_rid				= ops->get_rid;
	pd_data->phy_ops.vbus_on_check			= ops->vbus_on_check;
	pd_data->phy_ops.get_side_check			= ops->get_side_check;
	pd_data->phy_ops.set_power_role			= ops->set_power_role;
	pd_data->phy_ops.set_data_role			= ops->set_data_role;
	pd_data->phy_ops.set_vconn_source		= ops->set_vconn_source;
	pd_data->phy_ops.set_check_msg_pass		= ops->set_check_msg_pass;
	pd_data->phy_ops.set_otg_control		= ops->set_otg_control;
	pd_data->phy_ops.set_pd_control			= ops->set_pd_control;
	pd_data->phy_ops.set_rp_control			= ops->set_rp_control;
	pd_data->phy_ops.set_chg_lv_mode		= ops->set_chg_lv_mode;
	pd_data->phy_ops.pd_instead_of_vbus		= ops->pd_instead_of_vbus;
	pd_data->phy_ops.get_power_role			= ops->get_power_role;
	pd_data->phy_ops.get_data_role			= ops->get_data_role;
	pd_data->phy_ops.get_vconn_source		= ops->get_vconn_source;
	pd_data->phy_ops.tx_msg					= ops->tx_msg;
	pd_data->phy_ops.rx_msg					= ops->rx_msg;
#if defined(CONFIG_TYPEC)
	pd_data->phy_ops.set_pwr_opmode			= ops->set_pwr_opmode;
#endif
#if defined(CONFIG_SEC_FACTORY)
	pd_data->phy_ops.power_off_water_check	= ops->power_off_water_check;
#endif
	pd_data->phy_ops.get_water_detect		= ops->get_water_detect;
#if defined(CONFIG_PDIC_PD30)
	pd_data->phy_ops.send_hard_reset_dc		= ops->send_hard_reset_dc;
	pd_data->phy_ops.force_pps_disable		= ops->send_hard_reset_dc;
	pd_data->phy_ops.send_psrdy				= ops->send_psrdy;
	pd_data->phy_ops.get_pps_voltage		= ops->get_pps_voltage;
	pd_data->phy_ops.pps_enable				= ops->pps_enable;
	pd_data->phy_ops.get_pps_enable			= ops->get_pps_enable;;
#endif
}

protocol_state usbpd_protocol_rx_layer_reset_for_receive(struct protocol_data *rx)
{
	struct usbpd_data *pd_data = protocol_rx_to_usbpd(rx);

	dev_info(pd_data->dev, "%s\n", __func__);
	/*
	rx_layer_init(protocol_rx);
	pd_data->protocol_tx.state = PRL_Tx_PHY_Layer_Reset;

	usbpd_rx_soft_reset(pd_data);
	*/
	return PRL_Rx_Layer_Reset_for_Receive;

	/*return PRL_Rx_Send_GoodCRC;*/
}

protocol_state usbpd_protocol_rx_wait_for_phy_message(struct protocol_data *rx)
{
	struct usbpd_data *pd_data = protocol_rx_to_usbpd(rx);
	protocol_state state = PRL_Rx_Wait_for_PHY_Message;

	pd_data->msg_received = 0;

	if (PDIC_OPS_PARAM2_FUNC(rx_msg, pd_data, &rx->msg_header, rx->data_obj)) {
		dev_err(pd_data->dev, "%s IO Error\n", __func__);
		return state;
	} else {
		if (rx->msg_header.word == 0) {
			dev_err(pd_data->dev, "%s No Message\n", __func__);
			return state; /* no message */
		} else if (PDIC_OPS_PARAM_FUNC(get_status, pd_data, MSG_SOFTRESET))	{
			dev_err(pd_data->dev, "[Rx] Got SOFTRESET.\n");
			state = PRL_Rx_Layer_Reset_for_Receive;
		} else {
#if 0
			if (rx->stored_message_id == rx->msg_header.msg_id)
				return state;
#endif
			dev_err(pd_data->dev, "[Rx] [0x%x] [0x%x]\n",
					rx->msg_header.word, rx->data_obj[0].object);
			/* new message is coming */
			state = PRL_Rx_Send_GoodCRC;
		}
	}
	return state;
}

protocol_state usbpd_protocol_rx_send_goodcrc(struct protocol_data *rx)
{
	/* Goodcrc sent by PDIC(HW) */
	return PRL_Rx_Check_MessageID;
}

protocol_state usbpd_protocol_rx_store_messageid(struct protocol_data *rx)
{
	struct usbpd_data *pd_data = protocol_rx_to_usbpd(rx);

	rx->stored_message_id = rx->msg_header.msg_id;
	usbpd_read_msg(pd_data);
/*
	return PRL_Rx_Wait_for_PHY_Message;
*/
	return PRL_Rx_Store_MessageID;
}

protocol_state usbpd_protocol_rx_check_messageid(struct protocol_data *rx)
{
#if 0
	protocol_state state;

	if (rx->stored_message_id == rx->msg_header.msg_id)
		state = PRL_Rx_Wait_for_PHY_Message;
	else
		state = PRL_Rx_Store_MessageID;
	return state;
#endif
	return PRL_Rx_Store_MessageID;
}

void usbpd_protocol_tx(struct usbpd_data *pd_data)
{
	struct protocol_data *tx = &pd_data->protocol_tx;
	protocol_state next_state = tx->state;
	protocol_state saved_state;

	do {
		saved_state = next_state;
		switch (next_state) {
		case PRL_Tx_PHY_Layer_Reset:
			next_state = usbpd_protocol_tx_phy_layer_reset(tx);
			break;
		case PRL_Tx_Wait_for_Message_Request:
			next_state = usbpd_protocol_tx_wait_for_message_request(tx);
			break;
		case PRL_Tx_Layer_Reset_for_Transmit:
			next_state = usbpd_protocol_tx_layer_reset_for_transmit(tx);
			break;
		case PRL_Tx_Construct_Message:
			next_state = usbpd_protocol_tx_construct_message(tx);
			break;
		case PRL_Tx_Wait_for_PHY_Response:
			next_state = usbpd_protocol_tx_wait_for_phy_response(tx);
			break;
		case PRL_Tx_Match_MessageID:
			next_state = usbpd_protocol_tx_match_messageid(tx);
			break;
		case PRL_Tx_Message_Sent:
			next_state = usbpd_protocol_tx_message_sent(tx);
			break;
		case PRL_Tx_Check_RetryCounter:
			next_state = usbpd_protocol_tx_check_retrycounter(tx);
			break;
		case PRL_Tx_Transmission_Error:
			next_state = usbpd_protocol_tx_transmission_error(tx);
			break;
		case PRL_Tx_Discard_Message:
			next_state = usbpd_protocol_tx_discard_message(tx);
			break;
		default:
			next_state = PRL_Tx_Wait_for_Message_Request;
			break;
		}
	} while (saved_state != next_state);

	tx->state = next_state;
}

void usbpd_protocol_rx(struct usbpd_data *pd_data)
{
	struct protocol_data *rx = &pd_data->protocol_rx;
	protocol_state next_state = rx->state;
	protocol_state saved_state;

	do {
		saved_state = next_state;
		switch (next_state) {
		case PRL_Rx_Layer_Reset_for_Receive:
			next_state = usbpd_protocol_rx_layer_reset_for_receive(rx);
			break;
		case PRL_Rx_Wait_for_PHY_Message:
			next_state = usbpd_protocol_rx_wait_for_phy_message(rx);
			break;
		case PRL_Rx_Send_GoodCRC:
			next_state = usbpd_protocol_rx_send_goodcrc(rx);
			break;
		case PRL_Rx_Store_MessageID:
			next_state = usbpd_protocol_rx_store_messageid(rx);
			break;
		case PRL_Rx_Check_MessageID:
			next_state = usbpd_protocol_rx_check_messageid(rx);
			break;
		default:
			next_state = PRL_Rx_Wait_for_PHY_Message;
			break;
		}
	} while (saved_state != next_state);
/*
	rx->state = next_state;
*/
	rx->state = PRL_Rx_Wait_for_PHY_Message;
}

void usbpd_read_msg(struct usbpd_data *pd_data)
{
	int i;

	pd_data->policy.rx_msg_header.word
		= pd_data->protocol_rx.msg_header.word;
	for (i = 0; i < USBPD_MAX_COUNT_MSG_OBJECT; i++) {
		pd_data->policy.rx_data_obj[i].object
			= pd_data->protocol_rx.data_obj[i].object;
	}
}

/* return 1: sent with goodcrc, 0: fail */
bool usbpd_send_msg(struct usbpd_data *pd_data, msg_header_type *header,
		data_obj_type *obj)
{
	int i;

	if (obj)
		for (i = 0; i < USBPD_MAX_COUNT_MSG_OBJECT; i++)
			pd_data->protocol_tx.data_obj[i].object = obj[i].object;
	else
		header->num_data_objs = 0;
	header->spec_revision = pd_data->specification_revision;
	pd_data->protocol_tx.msg_header.word = header->word;
	usbpd_protocol_tx(pd_data);

	if (pd_data->protocol_tx.status == MESSAGE_SENT)
		return true;
	else
		return false;
}

inline bool usbpd_send_ctrl_msg(struct usbpd_data *d, msg_header_type *h,
		unsigned msg, unsigned dr, unsigned pr)
{
	h->msg_type = msg;
	h->port_data_role = dr;
	h->port_power_role = pr;
	h->num_data_objs = 0;
	return usbpd_send_msg(d, h, 0);
}

/* return: 0 if timed out, positive is status */
inline unsigned int usbpd_wait_msg(struct usbpd_data *pd_data,
				unsigned msg_status, unsigned ms)
{
	unsigned long ret;

	ret = PDIC_OPS_PARAM_FUNC(get_status, pd_data, msg_status);
	if (ret) {
		pd_data->policy.abnormal_state = false;
		return ret;
	}

	pr_info("%s, %d\n", __func__, __LINE__);
	/* wait */
	reinit_completion(&pd_data->msg_arrived);
	pd_data->wait_for_msg_arrived = msg_status;
	ret = wait_for_completion_timeout(&pd_data->msg_arrived,
			msecs_to_jiffies(ms));

	if (pd_data->policy.state == 0) {
		dev_err(pd_data->dev, "%s : return for policy state error\n", __func__);
		pd_data->policy.abnormal_state = true;
		return 0;
	}

	pd_data->policy.abnormal_state = false;

	return PDIC_OPS_PARAM_FUNC(get_status, pd_data, msg_status);
}

void usbpd_rx_hard_reset(struct device *dev)
{
	struct usbpd_data *pd_data = dev_get_drvdata(dev);

	usbpd_reinit(dev);
	usbpd_policy_reset(pd_data, HARDRESET_RECEIVED);
}

void usbpd_rx_soft_reset(struct usbpd_data *pd_data)
{
	usbpd_reinit(pd_data->dev);
	usbpd_policy_reset(pd_data, SOFTRESET_RECEIVED);
}

void usbpd_reinit(struct device *dev)
{
	struct usbpd_data *pd_data = dev_get_drvdata(dev);

	usbpd_init_counters(pd_data);
	usbpd_init_protocol(pd_data);
	usbpd_init_policy(pd_data);
	usbpd_init_manager_val(pd_data);
	reinit_completion(&pd_data->msg_arrived);
	pd_data->wait_for_msg_arrived = 0;
	pd_data->is_prswap = false;
	complete(&pd_data->msg_arrived);
}

/*
 * usbpd_init - alloc usbpd data
 *
 * Returns 0 on success; negative errno on failure
*/
int usbpd_init(struct device *dev, void *phy_driver_data)
{
	struct usbpd_data *pd_data;
	int ret = 0;

	if (!dev)
		return -EINVAL;

	pd_data = kzalloc(sizeof(struct usbpd_data), GFP_KERNEL);

	if (!pd_data)
		return -ENOMEM;

	pd_data->dev = dev;
	pd_data->phy_driver_data = phy_driver_data;
	dev_set_drvdata(dev, pd_data);

#if IS_ENABLED(CONFIG_CCIC_SYSFS)
	pr_info("%s, create ccic sysfs\n", __func__);
	/* create sysfs group */
	pd_data->ccic_dev = s2m_device_create(NULL, "ccic");
	if (IS_ERR(pd_data->ccic_dev))
		pr_err("%s Failed to create device(switch)!\n", __func__);
	else {
		/* create sysfs group */
		ret = sysfs_create_group(&pd_data->ccic_dev->kobj, &ccic_sysfs_group);
		if (ret)
			pr_err("%s: ccic sysfs fail, ret %d", __func__, ret);
		else
			dev_set_drvdata(pd_data->ccic_dev, pd_data);
	}
#endif

#if IS_ENABLED(CONFIG_IFCONN_NOTIFIER)
	pd_noti.pd_data = pd_data;
	pd_noti.pusbpd = pd_data;
	pd_noti.sink_status.current_pdo_num = 0;
	pd_noti.sink_status.selected_pdo_num = 0;
#else
	pd_noti.pusbpd = pd_data;
	pd_noti.sink_status.current_pdo_num = 0;
	pd_noti.sink_status.selected_pdo_num = 0;
#endif
	usbpd_init_counters(pd_data);
	usbpd_init_protocol(pd_data);
	usbpd_init_policy(pd_data);
	usbpd_init_manager(pd_data);

	mutex_init(&pd_data->accept_mutex);
	pd_data->policy_wake = wakeup_source_register(NULL, "policy_wake"); // 5.4 R

	pd_data->pd_support = 0;
#if defined(CONFIG_PDIC_SUPPORT_DP)
    pd_data->dp_enabled = 0;
#else
    pd_data->dp_enabled = 1;
#endif
    pd_data->is_otg_vboost = false;


	pd_data->policy_wqueue =
		create_singlethread_workqueue(dev_name(dev));
	if (!pd_data->policy_wqueue)
		pr_err("%s: Fail to Create Workqueue\n", __func__);

	INIT_WORK(&pd_data->worker, usbpd_policy_work);

	init_completion(&pd_data->msg_arrived);

	ret = usbpd_manager_psy_init(pd_data, dev);
	if (ret < 0) {
		pr_err("faled to register the ccic psy.\n");
	}

	usbpd_set_wake_lock(pd_data);

	return 0;
}
