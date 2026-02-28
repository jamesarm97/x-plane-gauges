#include "gauge_picker.h"
#include "gauges/gauge_registry.h"

// Static constexpr definitions
constexpr GaugePicker::Category GaugePicker::CATEGORIES[];

void GaugePicker::begin(DashboardLayout* layout) {
    _layout = layout;
    _open = false;
    _selectedCell = -1;
}

void GaugePicker::update() {
    if (_open && (millis() - _openTime > PICKER_TIMEOUT_MS)) {
        close();
    }
}

int GaugePicker::computeContentHeight() const {
    int h = 4;  // top padding
    for (int cat = 0; cat < CATEGORY_COUNT; cat++) {
        h += CAT_HEADER_H;
        h += CATEGORIES[cat].count * PICKER_ROW_HEIGHT;
    }
    return h;
}

bool GaugePicker::handleTouch(int x, int y, bool pressed) {
    bool justPressed = pressed && !_wasTouched;
    bool justReleased = !pressed && _wasTouched;
    _wasTouched = pressed;

    if (!_open) {
        // Picker is closed — check for cell tap
        if (justPressed) {
            int cell = hitTestCell(x, y);
            if (cell >= 0) {
                open(cell);
            }
        }
        return false;
    }

    // Picker is open
    if (justPressed) {
        _openTime = millis();   // Reset timeout on any touch
        _touchStartY = y;
        _lastTouchY = y;
        _isDragging = false;
    }

    if (pressed) {
        int dy = y - _lastTouchY;
        // Start dragging if moved beyond threshold
        if (!_isDragging && abs(y - _touchStartY) > DRAG_THRESHOLD) {
            _isDragging = true;
        }
        if (_isDragging) {
            _scrollOffset -= dy;
            if (_scrollOffset < 0) _scrollOffset = 0;
            if (_scrollOffset > _maxScroll) _scrollOffset = _maxScroll;
        }
        _lastTouchY = y;
    }

    if (justReleased) {
        if (!_isDragging) {
            // Tap (not drag) — select gauge or close
            if (x >= PANEL_X && x < PANEL_X + PANEL_W &&
                y >= PANEL_Y && y < PANEL_Y + PANEL_H) {
                if (y >= PANEL_Y + HEADER_H) {
                    int row = hitTestPickerRow(x, y);
                    if (row >= 0 && row < GAUGE_COUNT) {
                        _layout->setGauge(_selectedCell, row);
                        close();
                        return true;    // Gauge changed
                    }
                }
            } else {
                // Tap outside popup — close
                close();
                return false;
            }
        }
        _isDragging = false;
    }

    return false;
}

void GaugePicker::draw(LGFX_Sprite& fb) {
    if (!_open) return;

    // Darken background around popup
    fb.fillRect(PANEL_X - 4, PANEL_Y - 4, PANEL_W + 8, PANEL_H + 8, COLOR_BG);

    // Popup background with rounded corners and border
    fb.fillRoundRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 8, COLOR_PICKER_BG);
    fb.drawRoundRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 8, COLOR_PICKER_HL);

    // Header (fixed, not scrolled)
    fb.setTextDatum(middle_center);
    fb.setTextColor(COLOR_PICKER_HL);
    fb.setTextSize(2.2);
    char header[32];
    snprintf(header, sizeof(header), "CELL %d", _selectedCell + 1);
    fb.drawString(header, PANEL_X + PANEL_W / 2, PANEL_Y + HEADER_H / 2);

    // Separator
    fb.drawFastHLine(PANEL_X + 4, PANEL_Y + HEADER_H, PANEL_W - 8, COLOR_PICKER_HL);

    // Clip region for scrollable content
    int contentY = PANEL_Y + HEADER_H + 1;
    int contentH = PANEL_H - HEADER_H - 1;
    fb.setClipRect(PANEL_X, contentY, PANEL_W, contentH);

    // Gauge list with categories (scrollable)
    int yPos = contentY + 4 - _scrollOffset;
    int currentGauge = _layout->gaugeIndex(_selectedCell);

    for (int cat = 0; cat < CATEGORY_COUNT; cat++) {
        // Category header
        if (yPos + CAT_HEADER_H > contentY && yPos < PANEL_Y + PANEL_H) {
            fb.setTextDatum(middle_left);
            fb.setTextColor(COLOR_PICKER_HL);
            fb.setTextSize(1.5);
            fb.drawString(CATEGORIES[cat].name, PANEL_X + 12, yPos + CAT_HEADER_H / 2);
        }
        yPos += CAT_HEADER_H;

        // Gauge entries in this category
        for (int i = 0; i < CATEGORIES[cat].count; i++) {
            if (yPos > PANEL_Y + PANEL_H) break;  // Below visible area

            if (yPos + PICKER_ROW_HEIGHT > contentY) {
                int gaugeIdx = CATEGORIES[cat].startIdx + i;
                const GaugeConfig& cfg = g_gauges[gaugeIdx]->getConfig();
                bool isSelected = (gaugeIdx == currentGauge);

                if (isSelected) {
                    fb.fillRect(PANEL_X + 4, yPos + 2, PANEL_W - 8, PICKER_ROW_HEIGHT - 4, 0x0014);
                }

                fb.setTextDatum(middle_left);
                fb.setTextColor(isSelected ? COLOR_PICKER_HL : COLOR_PICKER_FG);
                fb.setTextSize(1.8);
                fb.drawString(cfg.title, PANEL_X + 20, yPos + PICKER_ROW_HEIGHT / 2);

                // Units on the right
                if (cfg.units[0] != '\0') {
                    fb.setTextDatum(middle_right);
                    fb.setTextColor(COLOR_DIAL_RIM);
                    fb.setTextSize(1.2);
                    fb.drawString(cfg.units, PANEL_X + PANEL_W - 16, yPos + PICKER_ROW_HEIGHT / 2);
                }
            }

            yPos += PICKER_ROW_HEIGHT;
        }
    }

    fb.clearClipRect();

    // Scroll indicator (bar on right edge)
    if (_maxScroll > 0) {
        int viewH = PANEL_H - HEADER_H;
        int totalContentH = computeContentHeight();
        int barH = max(30, viewH * viewH / totalContentH);
        int barY = PANEL_Y + HEADER_H + (_scrollOffset * (viewH - barH)) / _maxScroll;
        fb.fillRoundRect(PANEL_X + PANEL_W - 8, barY, 5, barH, 2, COLOR_DIAL_RIM);
    }
}

void GaugePicker::open(int cellIndex) {
    _open = true;
    _selectedCell = cellIndex;
    _scrollOffset = 0;
    _isDragging = false;
    _wasTouched = false;
    _openTime = millis();

    // Compute max scroll
    int contentH = computeContentHeight();
    int viewH = PANEL_H - HEADER_H;
    _maxScroll = max(0, contentH - viewH);
}

void GaugePicker::close() {
    _open = false;
    _selectedCell = -1;
    _wasTouched = false;
}

int GaugePicker::hitTestCell(int x, int y) {
    int col = x / CELL_WIDTH;
    int row = y / CELL_HEIGHT;
    if (col < 0 || col >= GRID_COLS || row < 0 || row >= GRID_ROWS) return -1;
    return row * GRID_COLS + col;
}

int GaugePicker::hitTestPickerRow(int x, int y) {
    if (x < PANEL_X || x >= PANEL_X + PANEL_W) return -1;

    int contentY = PANEL_Y + HEADER_H;
    if (y < contentY) return -1;

    // Walk through categories + rows with scroll offset applied
    int yPos = contentY + 4 - _scrollOffset;
    for (int cat = 0; cat < CATEGORY_COUNT; cat++) {
        yPos += CAT_HEADER_H;  // Category header

        for (int i = 0; i < CATEGORIES[cat].count; i++) {
            if (y >= yPos && y < yPos + PICKER_ROW_HEIGHT) {
                return CATEGORIES[cat].startIdx + i;
            }
            yPos += PICKER_ROW_HEIGHT;
        }
    }

    return -1;
}
