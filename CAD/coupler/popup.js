document.addEventListener('DOMContentLoaded', () => {
    // Updated element IDs
    const pendingList = document.getElementById('pendingList');
    const modelList = document.getElementById('modelList');
    const footprintList = document.getElementById('footprintList');
    const symbolList = document.getElementById('symbolList');
    const folderNameInput = document.getElementById('folderName');

    // Updated button IDs
    const clearPendingBtn = document.getElementById('clearPending');
    const clearModelBtn = document.getElementById('clearModel');
    const clearFootprintBtn = document.getElementById('clearFootprint');
    const clearSymbolBtn = document.getElementById('clearSymbol');
    const downloadAllBtn = document.getElementById('downloadAll');
    const clearAllCategorizedBtn = document.getElementById('clearAllCategorized'); // Updated ID

    // Updated storage key for pending links
    const storageKeys = ['linksToCategorize', 'modelLinks', 'footprintLinks', 'symbolLinks', 'downloadFolderName'];

    // --- Load initial state from storage ---
    chrome.storage.local.get(storageKeys, (result) => {
        // Render the pending list using the new key
        renderList(pendingList, result.linksToCategorize || [], 'pending');
        renderList(modelList, result.modelLinks || [], 'model');
        renderList(footprintList, result.footprintLinks || [], 'footprint');
        renderList(symbolList, result.symbolLinks || [], 'symbol');
        folderNameInput.value = result.downloadFolderName || '__SimonShark1__';
    });

    // --- Save folder name on change ---
    folderNameInput.addEventListener('input', () => {
        chrome.storage.local.set({ downloadFolderName: folderNameInput.value });
    });

    // --- Render Functions ---
    function renderList(ulElement, links, type) {
        ulElement.innerHTML = ''; // Clear existing items
        if (!links || links.length === 0) {
            const placeholder = type === 'pending' ? 'Right-click links and choose "Send Link to Holder"' : 'No links here.';
            ulElement.innerHTML = `<li><i>${placeholder}</i></li>`;
            return;
        }
        links.forEach((linkData, index) => {
            const li = document.createElement('li');
            // Ensure we handle the object structure { url: '...', id: ... }
            const linkUrl = linkData.url;
            const linkId = linkData.id;

            if (!linkUrl || !linkId) { // Safety check
                console.warn("Skipping invalid link data:", linkData);
                return;
            }


            const textSpan = document.createElement('span');
            textSpan.textContent = shortenUrl(linkUrl);
            textSpan.title = linkUrl; // Show full URL on hover
            li.appendChild(textSpan);

            // Updated type check for pending links
            if (type === 'pending') {
                // Add buttons to categorize
                const modelBtn = createButton('Model', () => assignLink(linkData, 'modelLinks'));
                const footprintBtn = createButton('Footprint', () => assignLink(linkData, 'footprintLinks'));
                const symbolBtn = createButton('Symbol', () => assignLink(linkData, 'symbolLinks'));
                 // Use 'linksToCategorize' as the key for removing pending links
                const discardBtn = createButton('X', () => removeLink('linksToCategorize', linkId), 'clear-item-btn', 'Discard this link');

                li.appendChild(modelBtn);
                li.appendChild(footprintBtn);
                li.appendChild(symbolBtn);
                li.appendChild(discardBtn);
            } else {
                // Add a clear button for categorized links
                // Ensure the correct listKey is used (e.g., 'modelLinks')
                const clearBtn = createButton('X', () => removeLink(`${type}Links`, linkId), 'clear-item-btn', `Remove this ${type} link`);
                li.appendChild(clearBtn);
            }
            ulElement.appendChild(li);
        });
    }

    function createButton(text, onClick, className = 'category-btn', title = '') {
        const button = document.createElement('button');
        button.textContent = text;
        button.addEventListener('click', onClick);
        if (className) button.classList.add(className);
        if (title) button.title = title;
        return button;
    }

    function shortenUrl(url, maxLength = 50) {
         if (!url) return '';
         if (url.length <= maxLength) {
             return url;
         }
         try {
            const parsedUrl = new URL(url);
            const pathParts = parsedUrl.pathname.split('/').filter(Boolean);
            const file = pathParts.pop() || '';
            const domain = parsedUrl.hostname;
            const displayPath = parsedUrl.pathname;

            // Try to show domain + filename first if it fits
            const domainPlusFile = `${domain}/.../${file}`;
            if (file.length > 0 && domainPlusFile.length <= maxLength) {
                 return domainPlusFile;
            }

            // Otherwise, show domain + truncated path
            const maxPathLength = maxLength - domain.length - 1; // -1 for the slash
             if (displayPath.length > maxPathLength) {
                return `${domain}${displayPath.substring(0, maxPathLength - 3)}...`;
            } else {
                return domain + displayPath;
            }
         } catch(e) {
             // Fallback for invalid URLs or simple strings
             return url.length > maxLength ? url.substring(0, maxLength - 3) + '...' : url;
         }
     }


    // --- Action Functions ---

    async function assignLink(linkData, targetListKey) {
        // 1. Add to target list
        const targetResult = await chrome.storage.local.get({ [targetListKey]: [] });
        const targetLinks = targetResult[targetListKey];
        // Prevent duplicates in target list (based on URL)
        if (!targetLinks.some(link => link.url === linkData.url)) {
            targetLinks.push(linkData); // Keep the object structure {url, id}
            await chrome.storage.local.set({ [targetListKey]: targetLinks });
        } else {
            console.log("Link already exists in target list:", targetListKey);
        }

        // 2. Remove from pending list (using the correct key 'linksToCategorize')
        await removeLink('linksToCategorize', linkData.id, false); // Don't re-render yet

        // 3. Re-render all lists
        refreshAllLists();
    }

     async function removeLink(listKey, linkId, shouldRefresh = true) {
        const result = await chrome.storage.local.get({ [listKey]: [] });
        let links = result[listKey];
        // Filter by ID, ensuring link objects exist and have an 'id' property
        links = links.filter(link => link && typeof link === 'object' && link.id !== linkId);
        await chrome.storage.local.set({ [listKey]: links });

        if (shouldRefresh) {
            refreshAllLists(); // Update UI
        }
     }


    async function clearList(listKey) {
        await chrome.storage.local.set({ [listKey]: [] });
        refreshAllLists();
    }

    async function refreshAllLists() {
         const result = await chrome.storage.local.get(storageKeys);
         // Use updated key 'linksToCategorize' and element 'pendingList'
         renderList(pendingList, result.linksToCategorize || [], 'pending');
         renderList(modelList, result.modelLinks || [], 'model');
         renderList(footprintList, result.footprintLinks || [], 'footprint');
         renderList(symbolList, result.symbolLinks || [], 'symbol');
         // Note: The badge update is primarily handled by the background script now,
         // but calling it here ensures consistency if needed, although it might be redundant.
         // chrome.runtime.sendMessage({ type: 'UPDATE_BADGE' }); // Or call background function directly if possible/needed
    }

    function deriveFilename(url) {
        try {
            const parsedUrl = new URL(url);
            const pathname = parsedUrl.pathname;
            const parts = pathname.split('/');
            const filename = decodeURIComponent(parts[parts.length - 1] || 'download');
            return filename.replace(/[/\\?%*:|"<>]/g, '_');
        } catch (e) {
            console.warn("Could not parse URL for filename:", url, e);
            const simpleName = url.substring(url.lastIndexOf('/') + 1).split('?')[0];
            return (simpleName || 'download').replace(/[/\\?%*:|"<>]/g, '_');
        }
    }

    async function downloadAllCategorized() {
        const folder = folderNameInput.value.trim() || '__SimonShark1__';
        const result = await chrome.storage.local.get(['modelLinks', 'footprintLinks', 'symbolLinks']);
        const allLinksData = [
            ...(result.modelLinks || []),
            ...(result.footprintLinks || []),
            ...(result.symbolLinks || [])
        ];

        if (allLinksData.length === 0) {
            alert("No categorized links to download!");
            return;
        }

        alert(`Starting download of ${allLinksData.length} files into subfolder "${folder}" (within your main Downloads folder).`);

        let downloadedCount = 0;
        // Use Promise.all to handle downloads concurrently (or sequentially if needed)
        const downloadPromises = allLinksData.map(linkData => {
             const url = linkData.url;
             const filename = deriveFilename(url);
             const suggestedPath = `${folder}/${filename}`; // Path relative to Downloads dir
             console.log(`Downloading: ${url} as ${suggestedPath}`);
             return chrome.downloads.download({
                 url: url,
                 filename: suggestedPath,
                 conflictAction: 'uniquify' // Automatically rename if file exists
             }).then(downloadId => {
                 if (downloadId) {
                    downloadedCount++;
                    console.log(`Started download ${downloadId} for ${url}`);
                 } else {
                     // This case might happen if the download is immediately cancelled or fails very early
                     console.error(`Failed to get downloadId for ${url}`);
                     alert(`Failed to start download for: ${filename}\nCheck browser console (Ctrl+Shift+J) for details.`);
                 }
                 return downloadId;
             }).catch(error => {
                console.error(`Failed to download ${url}:`, error);
                alert(`Failed to start download for: ${filename}\nError: ${error.message}\nCheck browser console for details.`);
                return null; // Indicate failure for this specific download
             });
        });

        // Wait for all download *attempts* to be initiated
        await Promise.all(downloadPromises);

        console.log(`Attempted to start ${downloadedCount} out of ${allLinksData.length} downloads.`);
        // Optionally clear lists after attempting download
        // if (confirm("Downloads initiated. Clear the categorized lists now?")) {
        //     await clearList('modelLinks');
        //     await clearList('footprintLinks');
        //     await clearList('symbolLinks');
        // }
    }

    // --- Event Listeners for Buttons (using updated IDs) ---
    clearPendingBtn.addEventListener('click', () => clearList('linksToCategorize')); // Use correct key
    clearModelBtn.addEventListener('click', () => clearList('modelLinks'));
    clearFootprintBtn.addEventListener('click', () => clearList('footprintLinks'));
    clearSymbolBtn.addEventListener('click', () => clearList('symbolLinks'));

    clearAllCategorizedBtn.addEventListener('click', async () => { // Use correct ID
        if (confirm("Are you sure you want to clear ALL categorized links (Model, Footprint, Symbol)? Links pending categorization will remain.")) {
            await clearList('modelLinks');
            await clearList('footprintLinks');
            await clearList('symbolLinks');
        }
    });

    downloadAllBtn.addEventListener('click', downloadAllCategorized);

});