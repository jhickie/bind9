/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stdbool.h>
#include <stdlib.h>

#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/journal.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/types.h>

isc_mem_t *mctx = NULL;

/*
 * Logging categories: this needs to match the list in bin/named/log.c.
 */
static isc_logcategory_t categories[] = { { "", 0 },
					  { "client", 0 },
					  { "network", 0 },
					  { "update", 0 },
					  { "queries", 0 },
					  { "unmatched", 0 },
					  { "update-security", 0 },
					  { "query-errors", 0 },
					  { NULL, 0 } };

static isc_result_t
loadzone(dns_db_t **db, const char *origin, const char *filename) {
	isc_result_t result;
	dns_fixedname_t fixed;
	dns_name_t *name;

	name = dns_fixedname_initname(&fixed);

	result = dns_name_fromstring(name, origin, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_create(mctx, ZONEDB_DEFAULT, name, dns_dbtype_zone,
			       dns_rdataclass_in, 0, NULL, db);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_load(*db, filename, dns_masterformat_text, 0);
	if (result == DNS_R_SEENINCLUDE) {
		result = ISC_R_SUCCESS;
	}
	return (result);
}

int
main(int argc, char **argv) {
	isc_result_t result;
	char *origin, *file1, *file2, *journal;
	dns_db_t *olddb = NULL, *newdb = NULL;
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;

	if (argc != 5) {
		printf("usage: %s origin file1 file2 journal\n", argv[0]);
		return (1);
	}

	origin = argv[1];
	file1 = argv[2];
	file2 = argv[3];
	journal = argv[4];

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	isc_mem_create(&mctx);

	isc_log_registercategories(categories);
	dns_log_init();

	logconfig = isc_logconfig_get();
	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination, 0);

	result = isc_log_usechannel(logconfig, "stderr", NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = loadzone(&olddb, origin, file1);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "Couldn't load %s: ", file1);
		goto cleanup;
	}

	result = loadzone(&newdb, origin, file2);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "Couldn't load %s: ", file2);
		goto cleanup;
	}

	result = dns_db_diff(mctx, newdb, NULL, olddb, NULL, journal);

cleanup:
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "%s\n", isc_result_totext(result));
	}

	if (newdb != NULL) {
		dns_db_detach(&newdb);
	}
	if (olddb != NULL) {
		dns_db_detach(&olddb);
	}

	if (mctx != NULL) {
		isc_mem_destroy(&mctx);
	}

	return (result != ISC_R_SUCCESS ? 1 : 0);
}
