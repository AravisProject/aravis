// SPDX-FileCopyrightText: 2021 GNOME Foundation
//
// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-or-later

(function() {

const QUERY_TYPES = [
    "alias",
    "bitfield",
    "callback",
    "class",
    "constant",
    "content",
    "ctor",
    "domain",
    "enum",
    "function_macro",
    "function",
    "interface",
    "method",
    "property",
    "record",
    "signal",
    "type_func",
    "union",
    "vfunc",
];
const QUERY_PATTERN = new RegExp("^(" + QUERY_TYPES.join('|') + ")\\s*:\\s*", 'i');

const TYPE_NAMES = {
    "alias": "alias",
    "bitfield": "flags",
    "callback": "callback",
    "class": "class",
    "constant": "constant",
    "content": "content",
    "ctor": "constructor",
    "domain": "error",
    "enum": "enum",
    "function_macro": "macro",
    "function": "function",
    "interface": "interface",
    "method": "method",
    "property": "property",
    "record": "struct",
    "signal": "signal",
    "type_func": "function",
    "union": "union",
    "vfunc": "vfunc",
};

const TYPE_CLASSES = {
    "alias": "alias",
    "bitfield": "flags",
    "callback": "callback",
    "class": "class",
    "constant": "constant",
    "content": "extra_content",
    "ctor": "ctor",
    "domain": "domain",
    "enum": "enum",
    "function_macro": "function_macro",
    "function": "function",
    "interface": "interface",
    "method": "method",
    "property": "property",
    "record": "record",
    "signal": "signal",
    "type_func": "type_func",
    "union": "union",
    "vfunc": "vfunc",
};

const fzy = window.fzy;
const searchParams = getSearchParams();
const refs = {
    input: null,
    form: null,
    search: null,
    main: null,
    toc: null,
    clear: null,
};

let searchIndex = undefined;
let searchResults = [];
let activeTypeFilter = null;

// Exports
window.onInitSearch = onInitSearch;
window.clearSearch = clearSearch;

/* Event handlers */

function onInitSearch() {
    fetchJSON('index.json', onDidLoadSearchIndex);
}

function onDidLoadSearchIndex(data) {
    searchIndex = new SearchIndex(data)

    refs.input  = document.querySelector("#search-input");
    refs.form   = document.querySelector("#search-form");
    refs.search = document.querySelector("#search");
    refs.main   = document.querySelector("#main");
    refs.toc    = document.querySelector("#toc");
    refs.clear  = document.querySelector("#search-clear");
    refs.help   = document.querySelector("#search-help");

    attachInputHandlers();

    if (searchParams.q) {
        removeClass(refs.clear, "hidden");
        addClass(refs.help, "hidden");
        search(searchParams.q, searchParams.type || null);
    }
}

function getNakedUrl() {
    return window.location.href.split("?")[0].split("#")[0];
}

function onDidSearch() {
    const query = refs.input.value;
    if (query) {
        removeClass(refs.clear, "hidden");
        addClass(refs.help, "hidden");
        search(query);
    }
    else {
        clearSearch();
    }
}

function onDidSubmit(ev) {
    ev.preventDefault();
    if (searchResults.length == 1) {
        window.location.href = searchResults[0].href;
    }
}

function attachInputHandlers() {
    if (refs.input.value === "") {
        refs.input.value = searchParams.q || "";
    }

    refs.input.addEventListener('keyup', debounce(200, onDidSearch))
    refs.form.addEventListener('submit', onDidSubmit)

    refs.clear.addEventListener('click', function() {
        clearSearch();
        refs.input.focus();
    });
}

/* Searching */

function searchQuery(query) {
    const q = matchQuery(query);
    const docs = searchIndex.searchDocs(q.term, q.type);

    const results = docs.map(function({ doc, positions }) {
        return {
            name: doc.name,
            type: doc.type,
            text: getLabelForDocument(doc, searchIndex.meta, positions),
            href: getLinkForDocument(doc),
            summary: doc.summary,
            deprecated: doc.deprecated,
        };
    });

    return results;
}

function search(query, typeFilter = null) {
    activeTypeFilter = typeFilter;
    showResults(query, searchQuery(query));
}

/* Rendering */

function showSearchResults() {
    addClass(refs.main, "hidden");
    if (refs.toc) {
        addClass(refs.toc, "hidden");
    }
    removeClass(refs.search, "hidden");
}

function hideSearchResults() {
    addClass(refs.search, "hidden");
    removeClass(refs.main, "hidden");
    if (refs.toc) {
        removeClass(refs.toc, "hidden");
    }
}

function createResultsTitle(query, n_results) {
    // Ensure we're returning an escaped query string, to ensure we
    // prevent XSS vulnerabilities
    let header = document.createElement("div");
    header.className = "search-results-header";

    let h1 = document.createElement("h1");
    let text = document.createTextNode("Results for “" + query + "” (" + n_results + ")");
    h1.appendChild(text);
    header.appendChild(h1);

    let btn = document.createElement("button");
    btn.className = "search-clear-btn";
    btn.setAttribute("aria-label", "Clear search");
    btn.textContent = "✕";
    btn.addEventListener("click", function() {
        clearSearch();
        refs.input.focus();
    });
    header.appendChild(btn);

    return header;
}

function createResultsContent(results) {
    let search_results = document.createElement("div");
    search_results.setAttribute("id", "search-results");

    if (results.length === 0) {
        search_results.textContent = "No results found.";
    }
    else {
        let html = "<div class=\"results\"><dl>";
        results.forEach(function(item) {
            html += "<dt class=\"result " + TYPE_CLASSES[item.type] + "\">" +
                      "<a href=\"" + item.href + "\">" + item.text + "</a>" +
                      "&nbsp;<span class=\"result emblem " + TYPE_CLASSES[item.type] + "\">" + TYPE_NAMES[item.type] + "</span>";
            if (item.deprecated) {
                html += "&nbsp;<span class=\"emblem deprecated\">deprecated:&nbsp;" + item.deprecated + "</span>";
            }
            html += "</dt>" +
                    "<dd>" + item.summary + "</dd>";
        });
        html += "</dl></div>";

        search_results.innerHTML = html;
    }

    return search_results;
}

function showResults(query, allResults) {
    if (window.history && typeof window.history.pushState === "function") {
        let baseUrl = getNakedUrl();
        let extra = "?q=" + encodeURIComponent(refs.input.value);
        if (activeTypeFilter) {
            extra += "&type=" + encodeURIComponent(activeTypeFilter);
        }
        window.history.replaceState(refs.input.value, "", baseUrl + extra + window.location.hash);
    }

    const filteredResults = activeTypeFilter
        ? allResults.filter(r => r.type === activeTypeFilter)
        : allResults;

    searchResults = filteredResults;

    window.title = "Results for “" + query + "” (" + filteredResults.length + ")";
    window.scroll({ top: 0 });
    const children = [createResultsTitle(query, filteredResults.length)];
    const filterBar = createTypeFilter(query, allResults);
    if (filterBar) children.push(filterBar);
    children.push(createResultsContent(filteredResults));
    refs.search.replaceChildren(...children);
    showSearchResults();
}

function createTypeFilter(query, allResults) {
    if (allResults.length === 0) return null;

    const counts = {};
    allResults.forEach(function(item) {
        counts[item.type] = (counts[item.type] || 0) + 1;
    });

    const types = Object.keys(counts).sort((a, b) => counts[b] - counts[a]);
    if (types.length <= 1) return null;

    const filterBar = document.createElement("div");
    filterBar.setAttribute("id", "search-type-filter");

    const ul = document.createElement("ul");

    function makeItem(label, count, typeKey) {
        const li = document.createElement("li");
        const btn = document.createElement("button");
        btn.className = "type-filter-btn" + (activeTypeFilter === typeKey ? " active" : "");

        const labelSpan = document.createElement("span");
        labelSpan.className = "type-filter-label";
        labelSpan.textContent = label;

        const countSpan = document.createElement("span");
        countSpan.className = "type-filter-count";
        countSpan.textContent = count;

        btn.appendChild(labelSpan);
        btn.appendChild(countSpan);

        btn.addEventListener("click", function() {
            activeTypeFilter = typeKey;
            showResults(query, allResults);
        });

        li.appendChild(btn);
        ul.appendChild(li);
    }

    makeItem("all", allResults.length, null);
    types.forEach(function(type) {
        makeItem(TYPE_NAMES[type] || type, counts[type], type);
    });

    filterBar.appendChild(ul);
    return filterBar;
}

function clearSearch() {
    refs.input.value = "";
    activeTypeFilter = null;
    addClass(refs.clear, "hidden");
    removeClass(refs.help, "hidden");
    if (window.history && typeof window.history.pushState === "function") {
        let baseUrl = getNakedUrl();
        window.history.replaceState(refs.input.value, "", baseUrl + window.location.hash);
    }
    hideSearchResults();
}

/* Search data instance */

function SearchIndex(searchIndex) {
    this.symbols = searchIndex.symbols;
    this.meta = searchIndex.meta;
}
SearchIndex.prototype.searchDocs = function searchDocs(term, type) {
    const filteredSymbols = !type ?
        this.symbols :
        this.symbols.filter(s => s.type === type);
    const results = fzy.filter(term, filteredSymbols, doc => getTextForDocument(doc, this.meta));
    return results.map(i => {
        const text = getTextForDocument(i.item, this.meta);
        return {
            doc: i.item,
            positions: fzy.positions(term.toLowerCase(), text.toLowerCase()),
        };
    });
}


/* Search metadata selectors */

function getLinkForDocument(doc) {
    switch (doc.type) {
        case "alias": return "alias." + doc.name + ".html";
        case "bitfield": return "flags." + doc.name + ".html";
        case "callback": return "callback." + doc.name + ".html";
        case "class": return "class." + doc.name + ".html";
        case "class_method": return "class_method." + doc.struct_for + "." + doc.name + ".html";
        case "constant": return "const." + doc.name + ".html";
        case "content": return doc.href;
        case "ctor": return "ctor." + doc.type_name + "." + doc.name + ".html";
        case "domain": return "error." + doc.name + ".html";
        case "enum": return "enum." + doc.name + ".html";
        case "function": return "func." + doc.name + ".html";
        case "function_macro": return "func." + doc.name + ".html";
        case "interface": return "iface." + doc.name + ".html";
        case "method": return "method." + doc.type_name + "." + doc.name + ".html";
        case "property": return "property." + doc.type_name + "." + doc.name + ".html";
        case "record": return "struct." + doc.name + ".html";
        case "signal": return "signal." + doc.type_name + "." + doc.name + ".html";
        case "type_func": return "type_func." + doc.type_name + "." + doc.name + ".html";
        case "union": return "union." + doc.name + ".html";
        case "vfunc": return "vfunc." + doc.type_name + "." + doc.name + ".html";
    }
    return null;
}

function highlightPositions(text, positions) {
    const posSet = new Set(positions);
    let html = '';
    let inMark = false;
    for (let i = 0; i < text.length; i++) {
        const isMatch = posSet.has(i);
        if (isMatch && !inMark) { html += '<mark>'; inMark = true; }
        else if (!isMatch && inMark) { html += '</mark>'; inMark = false; }
        html += text[i];
    }
    if (inMark) html += '</mark>';
    return html;
}

function getLabelForDocument(doc, meta, positions) {
    switch (doc.type) {
        case "alias":
        case "bitfield":
        case "callback":
        case "class":
        case "domain":
        case "enum":
        case "interface":
        case "record":
        case "union":
            return "<code>" + highlightPositions(doc.ctype, positions) + "</code>";

        case "class_method":
        case "constant":
        case "ctor":
        case "function":
        case "function_macro":
        case "method":
        case "type_func":
            return "<code>" + highlightPositions(doc.ident, positions) + "</code>";

        // NOTE: meta.ns added for more consistent results, otherwise
        // searching for "Button" would return all signals, properties
        // and vfuncs (eg "Button.clicked") before the actual object
        // (eg "GtkButton") because "Button" matches higher with starting
        // sequences.
        case "property":
            return "<code>" + highlightPositions(meta.ns + doc.type_name + ":" + doc.name, positions) + "</code>";
        case "signal":
            return "<code>" + highlightPositions(meta.ns + doc.type_name + "::" + doc.name, positions) + "</code>";
        case "vfunc":
            return "<code>" + highlightPositions(meta.ns + doc.type_name + "." + doc.name, positions) + "</code>";

        case "content":
            return highlightPositions(doc.name, positions);
    }

    return null;
}

function getTextForDocument(doc, meta) {
    switch (doc.type) {
        case "alias":
        case "bitfield":
        case "callback":
        case "class":
        case "domain":
        case "enum":
        case "interface":
        case "record":
        case "union":
            return doc.ctype;

        case "class_method":
        case "constant":
        case "ctor":
        case "function":
        case "function_macro":
        case "method":
        case "type_func":
            return doc.ident;

        // NOTE: meta.ns added for more consistent results, otherwise
        // searching for "Button" would return all signals, properties
        // and vfuncs (eg "Button.clicked") before the actual object
        // (eg "GtkButton") because "Button" matches higher with starting
        // sequences.
        case "property":
            return meta.ns + doc.type_name + ":" + doc.name;
        case "signal":
            return meta.ns + doc.type_name + "::" + doc.name;
        case "vfunc":
            return meta.ns + doc.type_name + "." + doc.name;

        case "content":
            return doc.name;
    }

    return null;
}


// Helpers

function fetchJSON(url, callback) {
    const request = new XMLHttpRequest();
    request.open('GET', url, true);
    request.onreadystatechange = function() {
        if (request.readyState === XMLHttpRequest.DONE) {
            const status = request.status;

            if (status === 0 || (status >= 200 && status < 400)) {
                callback(JSON.parse(request.responseText));
            }
        }
    }
    request.send(null);
}

function getSearchParams() {
    const params = {};
    window.location.search.substring(1).split('&')
        .map(function(s) {
            const pair = s.split('=');
            params[decodeURIComponent(pair[0])] =
                typeof pair[1] === 'undefined' ? null : decodeURIComponent(pair[1].replace(/\+/g, '%20'));
        });
    return params;
}

function matchQuery(input) {
    let type = null
    let term = input

    const matches = term.match(QUERY_PATTERN);
    if (matches) {
        type = matches[1];
        term = term.substring(matches[0].length);
    }

    // Remove all spaces, fzy will handle things gracefully.
    term = term.replace(/\s+/g, '')

    return { type: type, term: term }
}

function debounce(delay, fn) {
    let timeout;
    let savedArgs

    return function() {
        const self = this;
        savedArgs = Array.prototype.slice.call(arguments);

        if (timeout) {
            clearTimeout(timeout);
        }

        timeout = setTimeout(function() {
            fn.apply(self, savedArgs)
            timeout = undefined
        }, delay)
    }
}

})()
