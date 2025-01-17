#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "mipi_tx.h"
#include "sample_dsi_panel.h"

#define PANLE_ADAPTIVITY 0
static int fd;

#define MAX_DSI_OPTIONS	128
typedef enum _ARG_TYPE_ {
	ARG_INT = 0,
	ARG_UINT,
	ARG_STRING,
} ARG_TYPE;

typedef struct _optionExt_ {
	struct option opt;
	int type;
	int64_t min;
	int64_t max;
	const char *help;
} optionExt;

static optionExt long_option_ext[] = {
	{{"laneid",    optional_argument, NULL, 'l'},   ARG_STRING,    0,   0,
		"laneid sequence by order"},
	{{"pnswap",       optional_argument, NULL, 'p'},   ARG_STRING,   0,   0,
		"pnswap sequence by order"},
	{{"dsi-control",     no_argument, NULL, 'd'}, ARG_STRING, 0,   0,
		"set/get dsi status or settings." },
	{{"help",      no_argument, NULL, 'h'},       ARG_STRING, 0,   0,
		"print usage."},
	{{NULL, 0, NULL, 0}, ARG_INT, 0, 0, "no param: just init the panel."}
};

int dsi_init(int devno, const struct dsc_instr *cmds, int size)
{
	int ret;

	for (int i = 0; i < size; i++) {
		const struct dsc_instr *instr = &cmds[i];
		struct cmd_info_s cmd_info = {
			.devno = devno,
			.cmd_size = instr->size,
			.data_type = instr->data_type,
			.cmd = (void *)instr->data
		};

		ret = mipi_tx_send_cmd(fd, &cmd_info);
		if (instr->delay)
			usleep(instr->delay * 1000);

		if (ret) {
			SAMPLE_PRT("dsi init failed at %d instr.\n", i);
			return ret;
		}
	}
	return ret;
}

#if 0
#if PANLE_ADAPTIVITY
static int dsi_get_panel_id(int devno, unsigned int *id)
{
	int ret = 0;
	unsigned char param[3] = {0xDA, 0xDB, 0xDC};
	unsigned char buf[4];

	for (int i = 0; i < 3; i++) {
		struct get_cmd_info_s get_cmd_info = {
			.devno = devno,
			.data_type = 0x06,
			.data_param = param[i],
			.get_data_size = 0x01,
			.get_data = buf
		};
		memset(buf, 0, sizeof(buf));

		ret = mipi_tx_recv_cmd(fd, &get_cmd_info);
		if (ret < 0) {
			SAMPLE_PRT("dsi get panel id fail.\n");
			return ret;
		}

		*id |= (buf[0] << (i * 8));
	}
	return ret;
}

static void dsi_panel_init_adaptivity(void)
{
	unsigned int panelid = 0;
	const struct combo_dev_cfg_s *dev_cfg   = NULL;
	const struct hs_settle_s *hs_timing_cfg = NULL;
	const struct dsc_instr *dsi_init_cmds   = NULL;
	int size = 0;

	//use one type panel's cfg to init
	mipi_tx_cfg(fd, (struct combo_dev_cfg_s *)&dev_cfg_hx8394_720x1280);

	dsi_get_panel_id(0, &panelid);
	printf("Panel ID: 0x%X\n", panelid);

	switch (panelid) {
	case 0xF9483: //EVB
		dev_cfg = &dev_cfg_hx8394_720x1280;
		hs_timing_cfg = &hs_timing_cfg_hx8394_720x1280;
		dsi_init_cmds = dsi_init_cmds_hx8394_720x1280;
		size = ARRAY_SIZE(dsi_init_cmds_hx8394_720x1280);
		memcpy(panel_name, "HX8394-720x1280", sizeof("HX8394-720x1280"));
	break;

	case 0xAA: //it's an example
		dev_cfg = &dev_cfg_ili9881c_720x1280;
		hs_timing_cfg = &hs_timing_cfg_ili9881c_720x1280;
		dsi_init_cmds = dsi_init_cmds_ili9881c_720x1280;
		size = ARRAY_SIZE(dsi_init_cmds_ili9881c_720x1280);
		memcpy(panel_name, "ILI9881C-720x1280", sizeof("ILI9881C-720x1280"));
	break;

	default:
	break;
	}

	if (panelid != 0xF9483)
		mipi_tx_cfg(fd, (struct combo_dev_cfg_s *)dev_cfg);
	dsi_init(0, dsi_init_cmds, size);
	mipi_tx_set_hs_settle(fd, hs_timing_cfg);
}
#else
static void dsi_panel_init(void)
{
	const struct combo_dev_cfg_s *dev_cfg   = NULL;
	const struct hs_settle_s *hs_timing_cfg = NULL;
	const struct dsc_instr *dsi_init_cmds   = NULL;
	int size = 0;

	dev_cfg = &dev_cfg_hx8394_720x1280;
	hs_timing_cfg = &hs_timing_cfg_hx8394_720x1280;
	dsi_init_cmds = dsi_init_cmds_hx8394_720x1280;
	size = ARRAY_SIZE(dsi_init_cmds_hx8394_720x1280);
	memcpy(panel_name, "HX8394-720x1280", sizeof("HX8394-720x1280"));

	mipi_tx_cfg(fd, (struct combo_dev_cfg_s *)dev_cfg);
	dsi_init(0, dsi_init_cmds, size);
	mipi_tx_set_hs_settle(fd, hs_timing_cfg);
}
#endif
#endif

void SAMPLE_DSI_Print_Control(void)
{
	printf(" 0: dcs send\n");
	printf(" 1: dcs get\n");
	printf(" 2: switch to lp\n");
	printf(" 3: switch to hs\n");
	printf(" 4: get hs settle settings\n");
	printf(" 5: set hs settle settings\n");
}

void printDsiHelp(char **argv)
{
	CVI_U32 idx;

	printf("// ------------------------------------------------\n");
	printf("%s --laneid=laneid sequence --pnswap=pnswap sequence\n", argv[0]);
	printf("EX.\n");
	printf(" %s -h\n", argv[0]);
	printf(" %s --laneid=1,2,0,-1,-1 --pnswap=1,1,0,0,0\n", argv[0]);
	printf(" %s -d\n", argv[0]);
	printf("// ------------------------------------------------\n");

	for (idx = 0; idx < sizeof(long_option_ext) / sizeof(optionExt); idx++) {
		if (long_option_ext[idx].opt.name == NULL)
			break;

		printf("--%s\n", long_option_ext[idx].opt.name);
		printf("    %s\n", long_option_ext[idx].help);
	}
}

CVI_S32 SAMPLE_MIPI_SET_LANEID(char* pLaneid)
{
	if (pLaneid ==NULL) {
		return CVI_FAILURE;
    }

	CVI_S32 lane_id[] = {0,0,0,0,0};
	CVI_S32 n = sscanf(pLaneid, "%02d,%02d,%02d,%02d,%02d", &lane_id[0], &lane_id[1],&lane_id[2],&lane_id[3],&lane_id[4]);

	if(n != sizeof(lane_id)/sizeof(CVI_S32)) {
		return CVI_FAILURE;
	}

	for(CVI_U32 i = 0; i < sizeof(lane_id)/sizeof(CVI_S32); i++) {
		if(lane_id[i] < -1 || lane_id[i] > 5) {
			return CVI_FAILURE;
		}
		dev_cfg_hx8394_720x1280.lane_id[i] = lane_id[i];
	}

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_MIPI_SET_PNSWAP(char* pPnswap)
{
	if (pPnswap ==NULL) {
		return CVI_FAILURE;
    }

	CVI_U32 pnswap[] = {0,0,0,0,0};
	CVI_S32 n = sscanf(pPnswap, "%02d,%02d,%02d,%02d,%02d", &pnswap[0], &pnswap[1],&pnswap[2],&pnswap[3],&pnswap[4]);

	if(n != sizeof(pnswap)/sizeof(CVI_U32)) {
		return CVI_FAILURE;
	}

	for(CVI_U32 i = 0; i < sizeof(pnswap)/sizeof(CVI_U32); i++) {
		if((pnswap[i] != 0) && (pnswap[i] != 1)) {
			return CVI_FAILURE;
		}
		dev_cfg_hx8394_720x1280.lane_pn_swap[i] = (bool)(pnswap[i]);
	}

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_MIPI_TX_ENABLE()
{
	fd = open(MIPI_TX_NAME, O_RDWR | O_NONBLOCK, 0);
	if (fd == -1) {
		SAMPLE_PRT("Cannot open '%s': %d, %s\n", MIPI_TX_NAME, errno, strerror(errno));
		return CVI_FAILURE;
	}
	mipi_tx_disable(fd);
	mipi_tx_cfg(fd, (struct combo_dev_cfg_s *)panel_desc.dev_cfg);
	dsi_init(0, panel_desc.dsi_init_cmds, panel_desc.dsi_init_cmds_size);
	mipi_tx_set_hs_settle(fd, panel_desc.hs_timing_cfg);
	mipi_tx_enable(fd);
	printf("Init for MIPI-Driver-%s\n", panel_desc.panel_name);
	return CVI_SUCCESS;
}

void SAMPLE_DSI_CONTROLE()
{
	do {
		CVI_U32 tmp;
		SAMPLE_DSI_Print_Control();
		printf(" others: exit\n");
		printf("input op: ");

		scanf("%d", &tmp);
		if (tmp == 0) {
			struct cmd_info_s cmd_info;
			CVI_U8 data[16] = { 0 };
			int len = 0;

			printf("data size: ");
			scanf("%d", &tmp);
			cmd_info.cmd_size = tmp;

			printf("data type: 0x");
			scanf("%x", &tmp);
			cmd_info.data_type = tmp;

			do {
				printf("data[%d]: 0x", len);
				scanf("%x", &tmp);
				data[len++] = tmp;
			} while (len < cmd_info.cmd_size);
			cmd_info.cmd = data;
			mipi_tx_send_cmd(fd, &cmd_info);
		} else if (tmp == 1) {
			struct get_cmd_info_s cmd_info;
			CVI_U8 data[4] = { 0 };

			printf("get data size: ");
			scanf("%d", &tmp);
			cmd_info.get_data_size = tmp;

			printf("data type: 0x");
			scanf("%x", &tmp);
			cmd_info.data_type = tmp;

			printf("data param: 0x");
			scanf("%x", &tmp);
			cmd_info.data_param = tmp;

			cmd_info.get_data = data;
			mipi_tx_recv_cmd(fd, &cmd_info);
			printf("data[0]: %#x [1]: %#x [2]: %#x [3]: %#x\n"
				, cmd_info.get_data[0], cmd_info.get_data[1]
				, cmd_info.get_data[2], cmd_info.get_data[3]);
		} else if (tmp == 2) {
			mipi_tx_disable(fd);
		} else if (tmp == 3) {
			mipi_tx_enable(fd);
		} else if (tmp == 4) {
			struct hs_settle_s hs_cfg;

			mipi_tx_get_hs_settle(fd, &hs_cfg);
			printf("prepare(%d) zero(%d) trail(%d)\n",
				hs_cfg.prepare, hs_cfg.zero, hs_cfg.trail);
		} else if (tmp == 5) {
			struct hs_settle_s hs_cfg;

			printf("prepare: ");
			scanf("%d", &tmp);
			hs_cfg.prepare = tmp;

			printf("zero: ");
			scanf("%d", &tmp);
			hs_cfg.zero = tmp;

			printf("trail: ");
			scanf("%d", &tmp);
			hs_cfg.trail = tmp;

			mipi_tx_set_hs_settle(fd, &hs_cfg);
		} else
			break;
	} while (1);
}

int main(int argc, char *argv[])
{
	if(argc == 1) {
		SAMPLE_MIPI_TX_ENABLE();
		return CVI_SUCCESS;
	}

	struct option long_options[MAX_DSI_OPTIONS + 1];
	CVI_S32 ch, idx, ret;
	bool use_input_para = false;

	memset((void *)long_options, 0, sizeof(long_options));

	for (idx = 0; idx < MAX_DSI_OPTIONS; idx++) {
		if (long_option_ext[idx].opt.name == NULL)
			break;

		if (idx >= MAX_DSI_OPTIONS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "too many options\n");
			return -1;
		}

		memcpy(&long_options[idx], &long_option_ext[idx].opt, sizeof(struct option));
	}

	optind = 0;
	while ((ch = getopt_long(argc, argv, "dh", long_options, &idx)) != -1) {
		switch (ch) {
		case 'l':
			ret = SAMPLE_MIPI_SET_LANEID(optarg);
			if(ret != CVI_SUCCESS) {
				CVI_TRACE_LOG(CVI_DBG_ERR, "invalid laneid parameter\n");
				return ret;
			}
			use_input_para = true;
			break;
		case 'p':
			ret = SAMPLE_MIPI_SET_PNSWAP(optarg);
			if(ret != CVI_SUCCESS) {
				CVI_TRACE_LOG(CVI_DBG_ERR, "invalid pnswap parameter\n");
				return ret;
			}
			use_input_para = true;
			break;
		case 'd':
			if(argc > 2) {
				CVI_TRACE_LOG(CVI_DBG_ERR, "usage:%s -d. -d can't use in the same time with other command\n", argv[0]);
				return CVI_FAILURE;
			}
			SAMPLE_DSI_CONTROLE();
			break;
		case 'h':
			printDsiHelp(argv);
			break;
		default:
			SAMPLE_PRT("ch = %c\n", ch);
			printDsiHelp(argv);
			break;
		}
	}

	if (optind < argc)
		printDsiHelp(argv);

	if(use_input_para) {
		SAMPLE_MIPI_TX_ENABLE();
	}
	return 0;
}
