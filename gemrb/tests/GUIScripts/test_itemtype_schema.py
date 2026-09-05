#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual InitItemTypes table selection and slot-bit conversion.

The table loader is a controlled boundary. Native EE metadata is synthetic;
the fallback input is GemRB's own shipped data. This does not exercise actor
usability or the inventory UI and is not a replacement for a full engine build.
"""

from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]

BOUNDARIES = r'''
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
struct TableMgr {
    using index_t = unsigned int;
    std::vector<std::string> columns;
    std::vector<std::vector<int>> rows;
    unsigned GetRowCount() const { return rows.size(); }
    unsigned GetColumnCount() const { return rows.empty() ? 0 : rows.front().size(); }
    unsigned GetColNamesCount() const { return columns.size(); }
    unsigned GetColumnIndex(const std::string& name) const {
        auto found = std::find(columns.begin(), columns.end(), name);
        return found == columns.end() ? unsigned(-1) : found-columns.begin();
    }
    template<class T> T QueryFieldSigned(unsigned row, unsigned column) const {
        return rows.at(row).at(column);
    }
};
using AutoTable = std::shared_ptr<TableMgr>;
constexpr unsigned SLOT_INVENTORY = 32768;
void ThrowException(const std::string& text) { throw std::runtime_error(text); }
struct GameData {
    AutoTable primary, fallback;
    int fallbacks = 0;
    AutoTable LoadTable(const std::string& name) {
        if (name == "itemtype") return primary;
        if (name == "gitemtyp") { ++fallbacks; return fallback; }
        throw std::runtime_error("unexpected table lookup");
    }
} data;
GameData* gamedata = &data;
struct Interface {
    struct { std::string GameType; } config;
    unsigned ItemTypes = 0;
    std::vector<unsigned> slotmatrix;
    void InitItemTypes();
};
AutoTable readTable(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read GemRB baseline");
    auto table = std::make_shared<TableMgr>();
    std::string line, word;
    std::getline(input, line); std::getline(input, line); std::getline(input, line);
    std::istringstream header(line);
    while (header >> word) table->columns.push_back(word);
    while (std::getline(input, line)) {
        std::istringstream row(line);
        if (!(row >> word)) continue;
        std::vector<int> values;
        int value;
        while (row >> value) values.push_back(value);
        if (values.size() != table->columns.size()) throw std::runtime_error("bad baseline row");
        table->rows.push_back(values);
    }
    return table;
}
'''

MAIN = r'''
int main(int argc, char** argv) {
    if (argc != 4) return 4;
    Interface engine;
    engine.config.GameType = argv[1];
    std::string schema = argv[2];
    data.primary = readTable(argv[3]);
    data.fallback = readTable(argv[3]);
    if (schema == "native" || schema == "native-trailing-header" || schema == "missing-fallback") {
        data.primary->columns = {"TAKESOUND", "DROPSOUND", "SLOT"};
        // Actual 2DAImporter retains a final empty header after trailing
        // whitespace, but strips the empty trailing field from data rows.
        if (schema == "native-trailing-header") data.primary->columns.push_back("");
        data.primary->rows.assign(40, {0, 0, -1});
        data.primary->rows[2][2] = 1;
    } else if (schema == "custom") {
        // A mod deliberately permits daggers in SHIELD, not WEAPON.
        data.primary->rows[16][2] = 1;
        data.primary->rows[16][8] = 0;
    } else if (schema == "three-column-custom") {
        data.primary->columns = {"HELMET", "ARMOR", "SHIELD"};
        data.primary->rows.assign(40, {1, 0, 0});
    }
    if (schema == "missing-primary") data.primary = nullptr;
    if (schema == "missing-fallback") data.fallback = nullptr;
    try {
        engine.InitItemTypes();
        std::cout << data.fallbacks << ' ' << engine.slotmatrix.at(16)
                  << ' ' << engine.slotmatrix.at(2) << '\n';
    } catch (const std::runtime_error& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
'''


class ItemTypeSchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='gemrb-itemtype-test-')
        cls.addClassCleanup(cls.temp.cleanup)
        directory = Path(cls.temp.name)
        source = (ROOT / 'core/Interface.cpp').read_text()
        start = source.index('void Interface::InitItemTypes()')
        end = source.index('\t//itemtype data stores', start)
        # Keep the actual table selection and bit-matrix conversion verbatim;
        # the remaining function initializes separate armor/slot metadata.
        actual = source[start:end] + '}\n'
        generated = directory / 'itemtype.cpp'
        generated.write_text(BOUNDARIES + actual + MAIN)
        cls.binary = directory / 'itemtype'
        cls.baseline = ROOT / 'unhardcoded/shared/gitemtyp.2da'
        command = shlex.split(os.environ.get('CXX', 'c++'))
        subprocess.run(command + ['-std=c++17', '-Wall', '-Wextra', str(generated),
                                  '-o', str(cls.binary)], check=True, capture_output=True, text=True)

    def run_case(self, family, schema, expected=None, error=None):
        result = subprocess.run([str(self.binary), family, schema, str(self.baseline)],
                                capture_output=True, text=True)
        if error:
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
            self.assertIn(error, result.stderr)
        else:
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual([int(value) for value in result.stdout.split()], expected)

    def test_native_bgee_metadata_uses_gemrb_slot_matrix(self):
        self.run_case('bgee', 'native', [1, 32768 | 256, 32768 | 2])

    def test_native_bg2ee_metadata_uses_gemrb_slot_matrix(self):
        self.run_case('bg2ee', 'native', [1, 32768 | 256, 32768 | 2])

    def test_native_trailing_header_empty_token_does_not_change_schema(self):
        for family in ('bgee', 'bg2ee'):
            with self.subTest(family=family):
                self.run_case(family, 'native-trailing-header', [1, 32768 | 256, 32768 | 2])

    def test_valid_custom_matrix_keeps_modified_slot_permissions(self):
        for family in ('bgee', 'bg2ee', 'bg2', 'bg1', 'iwd2'):
            with self.subTest(family=family):
                self.run_case(family, 'custom', [0, 32768 | 4, 32768 | 2])

    def test_three_columns_alone_do_not_trigger_fallback(self):
        self.run_case('bgee', 'three-column-custom', [0, 32768 | 1, 32768 | 1])

    def test_non_ee_loading_is_unchanged(self):
        for family in ('bg1', 'bg2', 'iwd', 'iwd2', 'pst'):
            with self.subTest(family=family):
                self.run_case(family, 'native', [0, 32768 | 4, 32768 | 4])

    def test_missing_primary_still_fails_explicitly(self):
        self.run_case('bgee', 'missing-primary', error='Could not open itemtype table.')

    def test_missing_fallback_fails_instead_of_parsing_native_schema(self):
        self.run_case('bgee', 'missing-fallback', error='Could not open GemRB itemtype slot matrix.')

    def test_fallback_matches_existing_gemrb_bg_matrices(self):
        def matrix(path):
            lines = path.read_text().splitlines()
            # InitItemTypes indexes numeric item types, not display row names;
            # BGEE labels its two final all-zero rows BROKEN1/BROKEN2 while
            # BG2EE labels the identical permissions BOOK/FAMILIAR.
            return [lines[2].split()] + [line.split()[1:] for line in lines[3:] if line.strip()]
        actual = matrix(self.baseline)
        for source in ('unhardcoded/bgee/itemtype.2da', 'override/bg2ee/itemtype.2da'):
            with self.subTest(source=source):
                self.assertEqual(actual, matrix(ROOT / source))


if __name__ == '__main__':
    unittest.main()
