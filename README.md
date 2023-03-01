# gmod-ytdl

A server-side module that exposes yt-dlp to GMod.

### Requirements

This module expects `yt-dlp.exe` or `yt-dlp_linux` to be in your server's `garrysmod/lua/bin` directory.

The latest release for yt-dlp can be found [here](https://github.com/yt-dlp/yt-dlp/releases).

### Usage

---

##### `table`, `string` YTDL.GetInfo( `string` request )

###### Description

Returns info from the request.

###### Arguments

1. `string` request - The url or Youtube search query to get info on

###### Returns

1. `table` - Info from the request (`nil` if the function failed)
2. `string` - Error message if the function failed (`nil` if the function was successful)

---

The request can be a url from any of [these sites](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md).

If a url is not detected in the request, the module will assume it is a Youtube search and return the first search result or nil and an error message if no results found.

### Example

```lua
require("ytdl")
if not YTDL then return end

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