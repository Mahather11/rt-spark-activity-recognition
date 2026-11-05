#include "Driver_SPI.h"
#include "main.h"

// SPI2 is connected to the RW007 Wi-Fi module.
extern SPI_HandleTypeDef hspi2;

static ARM_SPI_SignalEvent_t SPI_EventCallback = 0;
static uint32_t SPI_DefaultTxValue = 0U;

static void SPI_SignalTransferComplete(void)
{
    if (SPI_EventCallback != NULL) {
        SPI_EventCallback(ARM_SPI_EVENT_TRANSFER_COMPLETE);
    }
}

static ARM_DRIVER_VERSION SPI_GetVersion(void) {
    ARM_DRIVER_VERSION version;
    version.api = ARM_SPI_API_VERSION;
    version.drv = 0x0100;
    return version;
}

static ARM_SPI_CAPABILITIES SPI_GetCapabilities(void) {
    ARM_SPI_CAPABILITIES cap = {0};
    cap.simplex = 1;
    return cap;
}

static int32_t SPI_Initialize(ARM_SPI_SignalEvent_t cb_event) {
    SPI_EventCallback = cb_event;
    return ARM_DRIVER_OK;
}

static int32_t SPI_Uninitialize(void) {
    return ARM_DRIVER_OK;
}

static int32_t SPI_PowerControl(ARM_POWER_STATE state) {
    if (state == ARM_POWER_FULL) {
        HAL_SPI_Init(&hspi2);
    }
    return ARM_DRIVER_OK;
}

static int32_t SPI_Send(const void *data, uint32_t num) {
    if (HAL_SPI_Transmit(&hspi2, (uint8_t *)data, num, HAL_MAX_DELAY) != HAL_OK) {
        return ARM_DRIVER_ERROR;
    }
    SPI_SignalTransferComplete();
    return num;
}

static int32_t SPI_Receive(void *data, uint32_t num) {
    if (data == NULL) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }
    /* The pack requests a 0x0A0A default word for clocking read data. */
    if (hspi2.Init.DataSize == SPI_DATASIZE_16BIT) {
        uint16_t *rx = (uint16_t *)data;
        uint16_t tx = (uint16_t)SPI_DefaultTxValue;
        for (uint32_t i = 0U; i < num; i++) {
            if (HAL_SPI_TransmitReceive(&hspi2, (uint8_t *)&tx,
                                        (uint8_t *)&rx[i], 1U,
                                        HAL_MAX_DELAY) != HAL_OK) {
                return ARM_DRIVER_ERROR;
            }
        }
    } else {
        uint8_t *rx = (uint8_t *)data;
        uint8_t tx = (uint8_t)SPI_DefaultTxValue;
        for (uint32_t i = 0U; i < num; i++) {
            if (HAL_SPI_TransmitReceive(&hspi2, &tx, &rx[i], 1U,
                                        HAL_MAX_DELAY) != HAL_OK) {
                return ARM_DRIVER_ERROR;
            }
        }
    }
    SPI_SignalTransferComplete();
    return num;
}

static int32_t SPI_Transfer(const void *data_out, void *data_in, uint32_t num) {
    if (HAL_SPI_TransmitReceive(&hspi2, (uint8_t *)data_out, (uint8_t *)data_in, num, HAL_MAX_DELAY) != HAL_OK) {
        return ARM_DRIVER_ERROR;
    }
    SPI_SignalTransferComplete();
    return num;
}

static uint32_t SPI_GetDataCount(void) {
    return 0;
}

static int32_t SPI_Control(uint32_t control, uint32_t arg) {
    uint32_t mode = control & ARM_SPI_CONTROL_Msk;
    uint32_t bits = (control & ARM_SPI_DATA_BITS_Msk) >> ARM_SPI_DATA_BITS_Pos;
    uint32_t frame = control & ARM_SPI_FRAME_FORMAT_Msk;
    uint32_t prescaler;
    uint32_t pclk;

    if (control == ARM_SPI_SET_DEFAULT_TX_VALUE) {
        SPI_DefaultTxValue = arg;
        return ARM_DRIVER_OK;
    }
    if ((mode != ARM_SPI_MODE_MASTER) ||
        ((bits != 8U) && (bits != 16U)) ||
        (frame != ARM_SPI_CPOL0_CPHA0) ||
        ((control & ARM_SPI_BIT_ORDER_Msk) != ARM_SPI_MSB_LSB)) {
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    /* Select the fastest APB1 divisor that does not exceed the requested rate. */
    pclk = HAL_RCC_GetPCLK1Freq();
    if (arg == 0U) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }
    if ((pclk / 2U) <= arg)       { prescaler = SPI_BAUDRATEPRESCALER_2; }
    else if ((pclk / 4U) <= arg)  { prescaler = SPI_BAUDRATEPRESCALER_4; }
    else if ((pclk / 8U) <= arg)  { prescaler = SPI_BAUDRATEPRESCALER_8; }
    else if ((pclk / 16U) <= arg) { prescaler = SPI_BAUDRATEPRESCALER_16; }
    else if ((pclk / 32U) <= arg) { prescaler = SPI_BAUDRATEPRESCALER_32; }
    else if ((pclk / 64U) <= arg) { prescaler = SPI_BAUDRATEPRESCALER_64; }
    else if ((pclk / 128U) <= arg) { prescaler = SPI_BAUDRATEPRESCALER_128; }
    else                           { prescaler = SPI_BAUDRATEPRESCALER_256; }

    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = (bits == 16U) ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = prescaler;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_DeInit(&hspi2) != HAL_OK || HAL_SPI_Init(&hspi2) != HAL_OK) {
        return ARM_DRIVER_ERROR;
    }
    return ARM_DRIVER_OK;
}

static ARM_SPI_STATUS SPI_GetStatus(void) {
    ARM_SPI_STATUS status = {0};
    return status;
}

// Link the CMSIS Driver instance to the Wi-Fi driver
extern ARM_DRIVER_SPI Driver_SPI2;
ARM_DRIVER_SPI Driver_SPI2 = {
    SPI_GetVersion,
    SPI_GetCapabilities,
    SPI_Initialize,
    SPI_Uninitialize,
    SPI_PowerControl,
    SPI_Send,
    SPI_Receive,
    SPI_Transfer,
    SPI_GetDataCount,
    SPI_Control,
    SPI_GetStatus
};
