# gmod-ytdl

A module that exposes yt-dlp to GMod.

### Requirements

This module expects `yt-dlp.exe` or `yt-dlp_linux` to be in your server's `garrysmod/lua/bin` directory.

The latest release for yt-dlp can be found [here](https://github.com/yt-dlp/yt-dlp/releases).

### Usage

---

##### YTDL.GetInfo( `string` request, `function` callback )

###### Description

Retrieves info from the request.

###### Arguments

1. `string` request - The url or Youtube search query to get info on
2. `function` callback - The function to call when finished. Arguments are
    - `table` data - Info from the request (`nil` if the function failed)
    - `string` error - The error message (nil if the request was successful)

---

The request can be a url from any of [these sites](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md).

If a url is not detected in the request, the module will assume it is a Youtube search and return the first search result or nil and an error message if no results found.

### Example

```lua
require("ytdl")
```

```lua
-- returns a table containing info for the request
-- or nil and an error message if it failed
YTDL.GetInfo("https://www.youtube.com/watch?v=qQzdAsjWGPg", function(data, err)
    if data then
        print(data.title)
    else
        print(err)
    end
end)

-- same as above but searching Youtube
YTDL.GetInfo("frank sinatra my way 2008 remastered", function(data, err)
    if data then
        print(data.title)
    else
        print(err)
    end
end)
```
