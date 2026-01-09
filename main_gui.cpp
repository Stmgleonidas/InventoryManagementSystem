#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <string>
#include "database.h"

using namespace std;

// ---------- Globals ----------
HWND hTree;
HWND hList;
sqlite3* db = nullptr;

// =======================================================
//                  INPUT BOX (NO RC FILE)
// =======================================================

struct InputBoxData {
    wstring prompt;
    wstring result;
};

LRESULT CALLBACK InputBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    InputBoxData* data = (InputBoxData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        data = (InputBoxData*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);

        CreateWindow(L"STATIC", data->prompt.c_str(),
            WS_CHILD | WS_VISIBLE,
            10, 10, 260, 20, hwnd, nullptr, nullptr, nullptr);

        HWND hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 35, 260, 25, hwnd, (HMENU)1, nullptr, nullptr);

        CreateWindow(L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            60, 75, 70, 25, hwnd, (HMENU)IDOK, nullptr, nullptr);

        CreateWindow(L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE,
            150, 75, 70, 25, hwnd, (HMENU)IDCANCEL, nullptr, nullptr);

        SetFocus(hEdit);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buf[256];
            GetWindowText(GetDlgItem(hwnd, 1), buf, 256);
            data->result = buf;
            DestroyWindow(hwnd);
            return 0;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            data->result.clear();
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        data->result.clear();
        DestroyWindow(hwnd);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

wstring InputBox(HWND parent, const wstring& title, const wstring& prompt)
{
    InputBoxData data{ prompt, L"" };

    WNDCLASS wc{};
    wc.lpfnWndProc = InputBoxWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"InputBoxClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    static bool registered = false;
    if (!registered) {
        RegisterClass(&wc);
        registered = true;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        wc.lpszClassName,
        title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 150,
        parent, nullptr, wc.hInstance, &data);

    // make owned window (important for focus)
    SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)parent);

    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(parent, FALSE);

    MSG msg;
    while (IsWindow(hwnd) && GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);

    return data.result;
}

// =======================================================
//                    DATABASE UI
// =======================================================

void PopulateBoxes()
{
    TreeView_DeleteAllItems(hTree);

    const char* sql = "SELECT id, name FROM boxes;";
    sqlite3_stmt* stmt = nullptr;
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

    const char* sql =
        "SELECT items.name, inventory.quantity "
        "FROM inventory "
        "JOIN items ON items.id = inventory.item_id "
        "WHERE inventory.box_id = ?;";

    sqlite3_stmt* stmt = nullptr;
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

// =======================================================
//                   MAIN WINDOW
// =======================================================

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
        CreateWindow(L"BUTTON", L"Add Box", WS_CHILD | WS_VISIBLE,
            btnX, btnY, 100, btnHeight, hwnd, (HMENU)101, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"Delete Box", WS_CHILD | WS_VISIBLE,
            btnX + 110, btnY, 100, btnHeight, hwnd, (HMENU)102, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"Add Item", WS_CHILD | WS_VISIBLE,
            btnX + 220, btnY, 100, btnHeight, hwnd, (HMENU)103, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"Delete Item", WS_CHILD | WS_VISIBLE,
            btnX + 330, btnY, 100, btnHeight, hwnd, (HMENU)104, nullptr, nullptr);

        int listY = btnY + btnHeight + spacing;
        hList = CreateWindowEx(0, WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
            treeWidth, listY, width - treeWidth, height - listY, hwnd, nullptr, nullptr, nullptr);

        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

        LVCOLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.cx = 300; col.pszText = (LPWSTR)L"Item";
        ListView_InsertColumn(hList, 0, &col);
        col.cx = 100; col.pszText = (LPWSTR)L"Quantity";
        ListView_InsertColumn(hList, 1, &col);

        PopulateBoxes();
        return 0;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
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
            if (!sel) break;

            TVITEM tvi{}; tvi.hItem = sel; tvi.mask = TVIF_PARAM;
            TreeView_GetItem(hTree, &tvi);
            int boxId = (int)tvi.lParam;

            sqlite3_stmt* stmt;
            const char* sql = "DELETE FROM inventory WHERE box_id=?;";
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, boxId);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            sql = "DELETE FROM boxes WHERE id=?;";
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, boxId);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            PopulateBoxes();
            ListView_DeleteAllItems(hList);
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

            wchar_t buf[256];
            ListView_GetItemText(hList, sel, 0, buf, 256);
            string itemName(buf, buf + wcslen(buf));

            HTREEITEM selBox = TreeView_GetSelection(hTree);
            if (!selBox) break;

            TVITEM tvi{}; tvi.hItem = selBox; tvi.mask = TVIF_PARAM;
            TreeView_GetItem(hTree, &tvi);
            int boxId = (int)tvi.lParam;

            sqlite3_stmt* stmt;
            const char* sql = "SELECT id FROM items WHERE name=?;";
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, itemName.c_str(), -1, SQLITE_TRANSIENT);

            int itemId = -1;
            if (sqlite3_step(stmt) == SQLITE_ROW)
                itemId = sqlite3_column_int(stmt, 0);
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
            PopulateItems((int)ntv->itemNew.lParam);
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

// =======================================================
//                     ENTRY POINT
// =======================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    db = openDatabase("inventory.db");
    if (!db)
    {
        MessageBox(nullptr, L"Failed to open database.", L"Error", MB_ICONERROR);
        return 0;
    }

    createTables(db);

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"InventoryWin32";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, L"Inventory Explorer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        900, 600, nullptr, nullptr, hInst, nullptr);

    if (!hwnd)
    {
        MessageBox(nullptr, L"Window creation failed.", L"Error", MB_ICONERROR);
        return 0;
    }

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
