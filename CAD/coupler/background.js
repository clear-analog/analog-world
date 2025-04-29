// --- Context Menu Setup ---
const CONTEXT_MENU_ID = "SEND_TO_LINK_HOLDER";

// Function to create the context menu item
function setupContextMenu() {
  chrome.contextMenus.create({
    id: CONTEXT_MENU_ID,
    title: "Send Link to Holder",
    contexts: ["link"] // Show context menu item only for links
  });
   console.log("Context menu created.");
}

// Create menu item when the extension is installed or updated
chrome.runtime.onInstalled.addListener(() => {
  setupContextMenu();
  // Initialize storage keys if they don't exist
  chrome.storage.local.get(['linksToCategorize', 'modelLinks', 'footprintLinks', 'symbolLinks', 'downloadFolderName'], result => {
      if (!result.linksToCategorize) chrome.storage.local.set({ linksToCategorize: [] });
      if (!result.modelLinks) chrome.storage.local.set({ modelLinks: [] });
      if (!result.footprintLinks) chrome.storage.local.set({ footprintLinks: [] });
      if (!result.symbolLinks) chrome.storage.local.set({ symbolLinks: [] });
      if (!result.downloadFolderName) chrome.storage.local.set({ downloadFolderName: '__SimonShark1__' });
      updateBadge(); // Initial badge update
  });

});

// On browser startup (in case context menu was removed)
chrome.runtime.onStartup.addListener(() => {
    // Although context menus usually persist, it's good practice
    // setupContextMenu(); // Might not be needed if it persists reliably across sessions
    updateBadge(); // Update badge on startup
});


// --- Context Menu Click Handler ---
chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId === CONTEXT_MENU_ID && info.linkUrl) {
    console.log("Context menu clicked for link:", info.linkUrl);
    addLinkToCategorize(info.linkUrl);
  }
});

// --- Helper Function to Add Link ---
async function addLinkToCategorize(linkUrl) {
  try {
    const result = await chrome.storage.local.get({ linksToCategorize: [] });
    const linksToCategorize = result.linksToCategorize;

    // Avoid adding duplicates
    if (!linksToCategorize.some(link => link.url === linkUrl)) {
      linksToCategorize.push({ url: linkUrl, id: Date.now() }); // Add with unique ID
      await chrome.storage.local.set({ linksToCategorize: linksToCategorize });
      console.log("Stored link to categorize:", linkUrl);
      updateBadge(); // Update badge after adding
    } else {
        console.log("Link already in the list to categorize:", linkUrl)
    }
  } catch (error) {
      console.error("Error adding link to categorize:", error);
  }
}

// --- Badge Update ---
async function updateBadge() {
    try {
        const result = await chrome.storage.local.get({ linksToCategorize: [] });
        const count = result.linksToCategorize?.length || 0;
        chrome.action.setBadgeText({ text: count > 0 ? String(count) : '' });
        if (count === 0) {
            chrome.action.setBadgeBackgroundColor({ color: [0, 0, 0, 0] }); // Transparent
        } else {
            chrome.action.setBadgeBackgroundColor({ color: '#FFA500' }); // Orange for pending
        }
    } catch (error) {
        console.error("Error updating badge:", error);
         chrome.action.setBadgeText({ text: 'ERR' });
         chrome.action.setBadgeBackgroundColor({ color: '#FF0000' });
    }
}


// --- Listen for storage changes to update badge ---
// This ensures the badge updates if the popup clears the list
chrome.storage.onChanged.addListener((changes, namespace) => {
  if (namespace === 'local' && (changes.linksToCategorize || changes.modelLinks || changes.footprintLinks || changes.symbolLinks)) {
      updateBadge(); // Update badge whenever relevant storage changes
  }
});

// **Important:** No more download interception listener (`chrome.downloads.onDeterminingFilename`) needed here.