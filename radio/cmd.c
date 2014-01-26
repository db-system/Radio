/**
 * \file
 * <!--
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Copyright 2013 Daniele Basile <asterix@develer.com>
 *
 * -->
 *
 * \brief Radio commands.
 *
 * \author Daniele Basile <asterix24@gmail.com>
 *
 */

#include "protocol.h"
#include "iwdg.h"
#include "radio_cfg.h"

#include <cfg/debug.h>

#include <drv/timer.h>
#include <drv/rtc.h>

#include <string.h>

static Devices local_dev[CMD_DEVICES];

static int cmd_master_broadcast(KFile *fd, Protocol *proto)
{
	kprintf("Broadcast: ");
	uint8_t reply = PROTO_ACK;
	for (int i = 0; i < CMD_DEVICES; i++)
	{
		local_dev[i].time = rtc_time();
		if (local_dev[i].addr == proto->addr)
		{
			break;
		}
		else if (!local_dev[i].addr)
		{
			local_dev[i].addr =  proto->addr;
			local_dev[i].status = CMD_NEW_DEV;
			break;
		}
		else
		{
			reply = PROTO_NACK;
		}
	}

	kprintf("type[%d]addr[%d]reply[%d]\n", proto->type, proto->addr, reply);

	return protocol_sendByte(fd, proto, proto->addr, proto->type, reply);
}

static int cmd_master_data(KFile *_fd, Protocol *proto)
{
	kprintf("Data\n");
	Radio *fd = RADIO_CAST(_fd);
	protocol_decode(fd, proto);
	return 0;
}

static int cmd_master_sleep(KFile *fd, Protocol *proto)
{
	(void)fd;
	(void)proto;
	kputs("Sleep ACK\n");

	for (int i = 0; i < CMD_DEVICES; i++)
	{
		if (local_dev[i].addr == proto->addr)
		{
			local_dev[i].status = CMD_SLEEP_DEV;
			local_dev[i].time = rtc_time();
			break;
		}
	}
	return 0;
}

const Cmd master_cmd[] = {
	{ CMD_BROADCAST, cmd_master_broadcast },
	{ CMD_DATA,      cmd_master_data      },
	{ CMD_SLEEP,     cmd_master_sleep     },
	{ 0   , NULL }
};



static uint8_t slave_status;
static uint8_t retry;

static int cmd_slave_sleep(KFile *fd, Protocol *proto)
{
	slave_status = CMD_SLAVE_STATUS_SHUTDOWN;
	kputs("cmd sleep\n");
	return protocol_sendByte(fd, proto, proto->addr, proto->type, PROTO_ACK);
}

static int cmd_slave_broadcast(KFile *fd, Protocol *proto)
{
	(void)fd;
	if (proto->data[0] == PROTO_ACK)
	{
		slave_status = CMD_SLAVE_STATUS_WAIT;
		kputs("Broadcast ACK\n");
	}
	else
		kputs("Broadcast NACK\n");

	return 0;
}

static int cmd_slave_data(KFile *fd, Protocol *proto)
{
	kprintf("Send data:type[%d], addr[%d]\n", proto->type, proto->addr);
	memset(proto, 0, sizeof(Protocol));
	protocol_encode(proto);

	return protocol_send(fd, proto, radio_cfg_id(), CMD_DATA);
}

const Cmd slave_cmd[] = {
	{ CMD_BROADCAST, cmd_slave_broadcast },
	{ CMD_DATA,      cmd_slave_data      },
	{ CMD_SLEEP,     cmd_slave_sleep      },
	{ 0     , NULL }
};

void cmd_slavePoll(KFile *fd, Protocol *proto)
{
	if (slave_status == CMD_SLAVE_STATUS_WAIT)
	{
		kprintf("wait..(%ld)\n", rtc_time());
		return;
	}

	if ((slave_status == CMD_SLAVE_STATUS_SHUTDOWN) || (retry == 3))
	{
		rtc_setAlarm(30);
		kprintf("Spengo..\n");
		go_standby();
	}

	memset(proto, 0, sizeof(Protocol));
	protocol_encode(proto);
	int sent = protocol_send(fd, proto, radio_cfg_id(), CMD_BROADCAST);
	kprintf("Broadcast sent[%d] %s[%d]\n",
				proto->type, sent < 0 ? "Error!":"Ok", sent);

	retry += 1;
}

void cmd_poll(KFile *fd, Protocol *proto)
{
	memset(proto, 0, sizeof(Protocol));
	for (int i = 0; i < CMD_DEVICES; i++)
	{
		kprintf("%d: ", i);
		if (local_dev[i].addr)
		{
			kprintf("Addr[%d],St[%d],time[%ld]\n", local_dev[i].addr,
						local_dev[i].status, local_dev[i].time);

			if ((rtc_time() - local_dev[i].time) > CMD_TIMEOUT)
			{
				int sent = protocol_send(fd, proto, local_dev[i].addr, CMD_SLEEP);
				kprintf("Sleep sent[%d] %s[%d]\n",
							proto->type, sent < 0 ? "Error!":"Ok", sent);
				continue;
			}
		}
		else
		{
			kprintf("Empty\n");
		}
	}
	kputs("-----\n");
}


