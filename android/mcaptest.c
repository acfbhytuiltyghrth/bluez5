/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014 Intel Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <glib.h>

#include "btio/btio.h"
#include "lib/l2cap.h"
#include "android/mcap-lib.h"

enum {
	MODE_NONE,
	MODE_CONNECT,
	MODE_LISTEN,
};

static GMainLoop *mloop;

static int ccpsm = 0x1003, dcpsm = 0x1005;

static struct mcap_instance *mcap = NULL;
static struct mcap_mdl *mdl = NULL;
static uint16_t mdlid;

static int control_mode = MODE_LISTEN;
static int data_mode = MODE_LISTEN;

static struct mcap_mcl *mcl = NULL;

static void mdl_connected_cb(struct mcap_mdl *mdl, void *data)
{
	/* TODO */
	printf("%s\n", __func__);
}

static void mdl_closed_cb(struct mcap_mdl *mdl, void *data)
{
	/* TODO */
	printf("%s\n", __func__);
}

static void mdl_deleted_cb(struct mcap_mdl *mdl, void *data)
{
	/* TODO */
	printf("%s\n", __func__);
}

static void mdl_aborted_cb(struct mcap_mdl *mdl, void *data)
{
	/* TODO */
	printf("%s\n", __func__);
}

static uint8_t mdl_conn_req_cb(struct mcap_mcl *mcl, uint8_t mdepid,
				uint16_t mdlid, uint8_t *conf, void *data)
{
	printf("%s\n", __func__);

	return MCAP_SUCCESS;
}

static uint8_t mdl_reconn_req_cb(struct mcap_mdl *mdl, void *data)
{
	printf("%s\n", __func__);

	return MCAP_SUCCESS;
}

static void mcl_reconnected(struct mcap_mcl *mcl, gpointer data)
{
	/* TODO */
	printf("MCL reconnected unsupported\n");
}

static void mcl_disconnected(struct mcap_mcl *mcl, gpointer data)
{
	/* TODO */
	printf("MCL disconnected\n");
}

static void mcl_uncached(struct mcap_mcl *mcl, gpointer data)
{
	/* TODO */
	printf("MCL uncached unsupported\n");
}

static void connect_mdl_cb(struct mcap_mdl *mdl, GError *gerr, gpointer data)
{
	mdlid = mcap_mdl_get_mdlid(mdl);

	printf("MDL %d connected\n", mdlid);
}

static void create_mdl_cb(struct mcap_mdl *mcap_mdl, uint8_t type, GError *gerr,
								gpointer data)
{
	GError *err = NULL;

	if (gerr) {
		printf("MDL error: %s\n", gerr->message);

		return;
	}

	if (mdl)
		mcap_mdl_unref(mdl);

	mdl = mcap_mdl_ref(mcap_mdl);

	if (!mcap_connect_mdl(mdl, L2CAP_MODE_ERTM, dcpsm, connect_mdl_cb, NULL,
								NULL, &err)) {
		printf("Error connecting to mdl: %s\n", err->message);
		g_error_free(err);
	}
}

static void trigger_mdl_action(int mode)
{
	GError *gerr = NULL;
	gboolean ret;

	ret = mcap_mcl_set_cb(mcl, NULL, &gerr,
		MCAP_MDL_CB_CONNECTED, mdl_connected_cb,
		MCAP_MDL_CB_CLOSED, mdl_closed_cb,
		MCAP_MDL_CB_DELETED, mdl_deleted_cb,
		MCAP_MDL_CB_ABORTED, mdl_aborted_cb,
		MCAP_MDL_CB_REMOTE_CONN_REQ, mdl_conn_req_cb,
		MCAP_MDL_CB_REMOTE_RECONN_REQ, mdl_reconn_req_cb,
		MCAP_MDL_CB_INVALID);

	if (!ret && gerr) {
		printf("MCL cannot handle connection %s\n",
							gerr->message);
		g_error_free(gerr);
	}

	if (mode == MODE_CONNECT) {
		mcap_create_mdl(mcl, 1, 0, create_mdl_cb, NULL, NULL, &gerr);
		if (gerr) {
			printf("Could not connect MDL: %s\n", gerr->message);
			g_error_free(gerr);
		}
	}
}

static void mcl_connected(struct mcap_mcl *mcap_mcl, gpointer data)
{
	printf("%s\n", __func__);

	if (mcl)
		mcap_mcl_unref(mcl);

	mcl = mcap_mcl_ref(mcap_mcl);
	trigger_mdl_action(data_mode);
}

static void create_mcl_cb(struct mcap_mcl *mcap_mcl, GError *err, gpointer data)
{
	if (err) {
		printf("Could not connect MCL: %s\n", err->message);

		return;
	}

	if (mcl)
		mcap_mcl_unref(mcl);

	mcl = mcap_mcl_ref(mcap_mcl);
	trigger_mdl_action(data_mode);
}
static void usage(void)
{
	printf("mcaptest - MCAP testing ver %s\n", VERSION);
	printf("Usage:\n"
		"\tmcaptest <control_mode> <data_mode> [options]\n");
	printf("Control Link Mode:\n"
		"\t-c connect <dst_addr>\n");
	printf("Data Link Mode:\n"
		"\t-d connect\n");
	printf("Options:\n"
		"\t-i <hcidev>        HCI device\n"
		"\t-C <control_ch>    Control channel PSM\n"
		"\t-D <data_ch>       Data channel PSM\n");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ "connect_cl",	1, 0, 'c' },
	{ "connect_dl",	0, 0, 'd' },
	{ "control_ch",	1, 0, 'C' },
	{ "data_ch",	1, 0, 'D' },
	{ 0, 0, 0, 0 }
};
int main(int argc, char *argv[])
{
	GError *err = NULL;
	bdaddr_t src, dst;
	int opt;
	char bdastr[18];

	hci_devba(0, &src);
	bacpy(&dst, BDADDR_ANY);

	mloop = g_main_loop_new(NULL, FALSE);
	if (!mloop) {
		printf("Cannot create main loop\n");

		exit(1);
	}

	while ((opt = getopt_long(argc, argv, "+i:c:C:D:hd",
						main_options, NULL)) != EOF) {
		switch (opt) {
		case 'i':
			if (!strncmp(optarg, "hci", 3))
				hci_devba(atoi(optarg + 3), &src);
			else
				str2ba(optarg, &src);

			break;

		case 'c':
			control_mode = MODE_CONNECT;
			str2ba(optarg, &dst);

			break;

		case 'd':
			data_mode = MODE_CONNECT;

			break;

		case 'C':
			ccpsm = atoi(optarg);

			break;

		case 'D':
			dcpsm = atoi(optarg);

			break;

		case 'h':
		default:
			usage();
			exit(0);
		}
	}

	mcap = mcap_create_instance(&src, BT_IO_SEC_MEDIUM, ccpsm, dcpsm,
					mcl_connected, mcl_reconnected,
					mcl_disconnected, mcl_uncached,
					NULL, /* CSP is not used right now */
					NULL, &err);

	if (!mcap) {
		printf("MCAP instance creation failed %s\n", err->message);
		g_error_free(err);

		exit(1);
	}

	switch (control_mode) {
	case MODE_CONNECT:
		ba2str(&dst, bdastr);
		printf("Connecting to %s\n", bdastr);

		mcap_create_mcl(mcap, &dst, ccpsm, create_mcl_cb, NULL, NULL,
									&err);

		if (err) {
			printf("MCAP create error %s\n", err->message);
			g_error_free(err);

			exit(1);
		}

		break;
	case MODE_LISTEN:
		printf("Listening for control channel connection\n");

		break;
	case MODE_NONE:
	default:
		goto done;
	}

	g_main_loop_run(mloop);

done:
	printf("Done\n");

	if (mcap)
		mcap_instance_unref(mcap);

	g_main_loop_unref(mloop);

	return 0;
}