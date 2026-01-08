#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <string>
#include "database.h"
#include "resource.h"


using namespace std;

// ---------- Globals ----------
HWND hTree;
HWND hList;
sqlite3* db;

// ---------- InputBox using DialogBoxParam ----------

struct InputBoxData {
    wstring prompt;
    wstring result;
};


// Dialog procedure

INT_PTR CALLBACK InputBoxDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    InputBoxData* data = (InputBoxData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(msg)
    {
        case WM_INITDIALOG:
        {
            data = (InputBoxData*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            SetWindowText(GetDlgItem(hwnd, IDC_PROMPT), data->prompt.c_str());
            return TRUE;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    wchar_t buf[256];
                    GetWindowText(GetDlgItem(hwnd, IDC_EDIT), buf, 256);
                    data->result = buf;
                    EndDialog(hwnd, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}


// Show modal input box
wstring InputBox(HWND parent, const wstring& title, const wstring& prompt)
{
    InputBoxData data{ prompt, L"" };
    HINSTANCE hInst = GetModuleHandle(nullptr);

    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_INPUTBOX), parent, InputBoxDlgProc, (LPARAM)&data);

    return data.result;
}


// ---------- Helpers ----------
void PopulateBoxes()
{
    TreeView_DeleteAllItems(hTree);

    const char* sql = "SELECT id, name FROM boxes;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = (const char*)sqlite3_column_text(stmt, 1);

        wstring wname(name, name + strlen(name));

        TVINSERTSTRUCT tvi{};
        tvi.hParent = TVI_ROOT;
        tvi.hInsertAfter = TVI_LAST;
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvi.item.pszText = (LPWSTR)wname.c_str();
        tvi.item.lParam = id;

        TreeView_InsertItem(hTree, &tvi);
    }

    sqlite3_finalize(stmt);
}

void PopulateItems(int boxId)
{
    ListView_DeleteAllItems(hList);

    const char* sql = R"(
        SELECT items.name, inventory.quantity
        FROM inventory
        JOIN items ON items.id = inventory.item_id
        WHERE inventory.box_id = ?;
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, boxId);

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char* item = (const char*)sqlite3_column_text(stmt, 0);
        int qty = sqlite3_column_int(stmt, 1);

        wstring witem(item, item + strlen(item));

        LVITEM lvi{};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.pszText = (LPWSTR)witem.c_str();
        ListView_InsertItem(hList, &lvi);

        wchar_t buf[32];
        wsprintf(buf, L"%d", qty);
        ListView_SetItemText(hList, index, 1, buf);

        index++;
    }

    sqlite3_finalize(stmt);
}

// ---------- Window Proc ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES };
            InitCommonControlsEx(&icc);

            RECT rc; GetClientRect(hwnd, &rc);
            int width = rc.right, height = rc.bottom;
            int treeWidth = 200, btnHeight = 30, spacing = 5;

            hTree = CreateWindowEx(0, WC_TREEVIEW, L"",
                                   WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_SHOWSELALWAYS,
                                   0, 0, treeWidth, height, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

            int btnX = treeWidth + 10, btnY = 5;
            CreateWindow(L"BUTTON", L"Add Box", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         btnX, btnY, 100, btnHeight, hwnd, (HMENU)101, GetModuleHandle(nullptr), nullptr);
            CreateWindow(L"BUTTON", L"Delete Box", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         btnX + 110, btnY, 100, btnHeight, hwnd, (HMENU)102, GetModuleHandle(nullptr), nullptr);
            CreateWindow(L"BUTTON", L"Add Item", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         btnX + 220, btnY, 100, btnHeight, hwnd, (HMENU)103, GetModuleHandle(nullptr), nullptr);
            CreateWindow(L"BUTTON", L"Delete Item", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         btnX + 330, btnY, 100, btnHeight, hwnd, (HMENU)104, GetModuleHandle(nullptr), nullptr);

            int listY = btnY + btnHeight + spacing;
            hList = CreateWindowEx(0, WC_LISTVIEW, L"",
                                   WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
                                   treeWidth, listY, width - treeWidth, height - listY, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

            LVCOLUMN col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.cx = 300; col.pszText = (LPWSTR)L"Item";
            ListView_InsertColumn(hList, 0, &col);
            col.cx = 100; col.pszText = (LPWSTR)L"Quantity";
            ListView_InsertColumn(hList, 1, &col);

            PopulateBoxes();
            break;
        }

        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            switch (id)
            {
                case 101: // Add Box
                {
                    wstring name = InputBox(hwnd, L"Add Box", L"Box name:");
                    if (!name.empty())
                    {
                        addBox(db, string(name.begin(), name.end()));
                        PopulateBoxes();
                    }
                    break;
                }
                case 102: // Delete Box
                {
                    HTREEITEM sel = TreeView_GetSelection(hTree);
                    if (sel)
                    {
                        TVITEM tvi{}; tvi.hItem = sel; tvi.mask = TVIF_PARAM;
                        TreeView_GetItem(hTree, &tvi);
                        int boxId = (int)tvi.lParam;

                        const char* sql = "DELETE FROM inventory WHERE box_id=?;";
                        sqlite3_stmt* stmt;
                        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
                        sqlite3_bind_int(stmt, 1, boxId);
                        sqlite3_step(stmt); sqlite3_finalize(stmt);

                        sql = "DELETE FROM boxes WHERE id=?;";
                        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
                        sqlite3_bind_int(stmt, 1, boxId);
                        sqlite3_step(stmt); sqlite3_finalize(stmt);

                        PopulateBoxes();
                        ListView_DeleteAllItems(hList);
                    }
                    break;
                }
                case 103: // Add Item
                {
                    HTREEITEM selBox = TreeView_GetSelection(hTree);
                    if (!selBox) break;

                    TVITEM tvi{}; tvi.hItem = selBox; tvi.mask = TVIF_PARAM;
                    TreeView_GetItem(hTree, &tvi);
                    int boxId = (int)tvi.lParam;

                    wstring name = InputBox(hwnd, L"Add Item", L"Item name:");
                    if (!name.empty())
                    {
                        int itemId = addItem(db, string(name.begin(), name.end()));
                        addInventory(db, boxId, itemId, 1);
                        PopulateItems(boxId);
                    }
                    break;
                }
                case 104: // Delete Item
                {
                    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (sel == -1) break;

                    wchar_t buf[256]; ListView_GetItemText(hList, sel, 0, buf, 256);
                    string itemName(buf, buf + wcslen(buf));

                    HTREEITEM selBox = TreeView_GetSelection(hTree);
                    if (!selBox) break;

                    TVITEM tvi{}; tvi.hItem = selBox; tvi.mask = TVIF_PARAM;
                    TreeView_GetItem(hTree, &tvi);
                    int boxId = (int)tvi.lParam;

                    const char* sql = "SELECT id FROM items WHERE name=?;";
                    sqlite3_stmt* stmt;
                    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
                    sqlite3_bind_text(stmt, 1, itemName.c_str(), -1, SQLITE_TRANSIENT);
                    int itemId = -1;
                    if (sqlite3_step(stmt) == SQLITE_ROW) itemId = sqlite3_column_int(stmt, 0);
                    sqlite3_finalize(stmt);

                    if (itemId != -1)
                    {
                        deleteInventory(db, boxId, itemId);
                        PopulateItems(boxId);
                    }
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;
            if (hdr->hwndFrom == hTree && hdr->code == TVN_SELCHANGED)
            {
                NMTREEVIEW* ntv = (NMTREEVIEW*)lParam;
                int boxId = (int)ntv->itemNew.lParam;
                PopulateItems(boxId);
            }
            break;
        }

        case WM_SIZE:
        {
            int w = LOWORD(lParam), h = HIWORD(lParam);
            int treeWidth = 200, btnHeight = 30, btnY = 5, spacing = 5;
            int listY = btnY + btnHeight + spacing;

            MoveWindow(hTree, 0, 0, treeWidth, h, TRUE);
            MoveWindow(hList, treeWidth, listY, w - treeWidth, h - listY, TRUE);

            MoveWindow(GetDlgItem(hwnd, 101), treeWidth + 10, btnY, 100, btnHeight, TRUE);
            MoveWindow(GetDlgItem(hwnd, 102), treeWidth + 120, btnY, 100, btnHeight, TRUE);
            MoveWindow(GetDlgItem(hwnd, 103), treeWidth + 230, btnY, 100, btnHeight, TRUE);
            MoveWindow(GetDlgItem(hwnd, 104), treeWidth + 340, btnY, 100, btnHeight, TRUE);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

// ---------- Entry ----------
int main()
{
    HINSTANCE hInst = GetModuleHandle(nullptr);
    db = openDatabase("inventory.db");
    createTables(db);

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"InventoryWin32";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, L"Inventory Explorer",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             800, 600, nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    closeDatabase(db);
    return 0;
}
