YIELDED_COROUTINES = {
    -- {coro = coroutine, framesLeft = int, src = string}, ...
}
WAIT_LENGTH = 0

function Wait(seconds) 
    local _, isMain = coroutine.running()
    -- print("at WAIT()")
    assert(not isMain, "Somehow, you managed to run this code without being in a coroutine. Please report this.")
    WAIT_LENGTH = seconds * 60
    -- print("woahh ", coroutine.running())
    -- local ok, err = pcall(coroutine.yield)
    coroutine.yield()
    -- print("WE got ", ok, " and ", err)
    
end

-- function __FUNC_(src)
--     print("hi")
--     require("__OOPS_", src)
--     print("bruh")
-- end

function DoTask(func, src)
    
    print("Doing task on func ", func, " from file ", src)
    local co = coroutine.create(func)

    local ok, err = coroutine.resume(co)
    print("it finished, ok = ", tostring(ok), "err = ", tostring(err))

    -- coroutine will do some stuff then yield or finish
    if ok and coroutine.status(co) ~= "dead" then -- if coroutine didn't finish/error
        print("its alive?")
        table.insert(YIELDED_COROUTINES, {coro = co, framesLeft = 0, src = src})
        
    else
        if not ok then
            error((err)..debug.traceback(co))
        end
    end
    print("Finished task")
end

function ResumeTasks()
    for i, v in ipairs(YIELDED_COROUTINES) do
        if v.framesLeft <= 0 then
            local ok, err = coroutine.resume(v.coro)
            -- coroutine does some stuff then yield or finish
            if coroutine.status(v.coro) == "dead" then -- if coroutine is finished/errored, remove it from the list
                table.remove(YIELDED_COROUTINES, i)
                
                if not ok then
                    pcall(error, err)
                end
            else
                -- if the coroutine yielded by calling Wait(), WAIT_LENGTH will be set to the number of frames they want to wait for.
                -- if they called coroutine.yield() directly, it will be 0.
                v.framesLeft = WAIT_LENGTH
                WAIT_LENGTH = 0
            end
        else
            v.framesLeft = v.framesLeft - 1 -- lua doesn't support -= because mid
        end
    end
end