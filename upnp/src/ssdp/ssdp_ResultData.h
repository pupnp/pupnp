#ifndef SSDP_RESULTDATA_H
#define SSDP_RESULTDATA_H

/*!
 * \addtogroup SSDPlib
 *
 * @{
 * 
 * \file
 *
 * \brief SSDPResultData object declararion.
 *
 * \author Marcelo Roberto Jimenez
 */

/*! Structure to contain Discovery response. */
typedef struct resultData
{
	struct Upnp_Discovery param;
	void *cookie;
	Upnp_FunPtr ctrlpt_callback;
} ResultData;

/* @} SSDPlib */

#endif /* SSDP_RESULTDATA_H */
