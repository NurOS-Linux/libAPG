// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file json.h
 * @brief JSON serialization and deserialization for package types.
 */

#include "package.h"

/**
 * @brief Serialize a package to a JSON string.
 *
 * @param pkg Package to serialize.
 * @return Heap-allocated JSON string; caller must free().
 *         Returns NULL on serialization failure.
 */
char *package_to_json(struct package *pkg);

/**
 * @brief Deserialize a package from a JSON buffer.
 *
 * @param json JSON input buffer.
 * @param len  Length of @p json in bytes.
 * @return Heap-allocated package on success, NULL on parse failure.
 *         Caller must call package_free() when done.
 */
struct package *package_from_json(const char *json, size_t len);

/**
 * @brief Parse package metadata from a JSON file on disk.
 *
 * @param path Path to a JSON file containing package metadata.
 * @return Heap-allocated metadata on success, NULL on failure.
 *         Caller must call package_metadata_free() when done.
 */
struct package_metadata *package_metadata_from_file(const char *path);

/**
 * @brief Deserialize package metadata from a JSON buffer.
 *
 * @param json JSON input buffer.
 * @param len  Length of @p json in bytes.
 * @return Heap-allocated metadata on success, NULL on parse failure.
 *         Caller must call package_metadata_free() when done.
 */
struct package_metadata *package_metadata_from_json(const char *json, size_t len);
