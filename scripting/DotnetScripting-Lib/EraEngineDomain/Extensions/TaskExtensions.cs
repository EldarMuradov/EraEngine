using EraEngine;

namespace EraEngine.Extensions;

public static class TaskExtensions
{
    public static void FireAndForget(this Task task, Action<Exception> handler = null)
    {
        task.ContinueWith(t =>
        {
            if (t.IsFaulted && handler != null)
                handler(t.Exception);
        }, TaskContinuationOptions.OnlyOnFaulted).ConfigureAwait(false);
    }

    public static async Task OnFailure(this Task task, Action<Exception> handler)
    {
        try
        {
            await task.ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            handler(ex);
        }
    }

    public static async Task WithTimeout(this Task task, TimeSpan timeout)
    {
        var delay = Task.Delay(timeout);
        var completed = await Task.WhenAny(task, delay).ConfigureAwait(false);
        if (completed == delay)
            throw new TimeoutException();

        await task;
    }

    public static async Task<Result<TResult>> Fallback<TResult>(this Task<Result<TResult>> task, Result<TResult> fallback)
    {
        try
        {
            return await task.ConfigureAwait(false);
        }
        catch
        {
            return fallback;
        }
    }

    public static async Task<Result<TResult>> Retry<TResult>(this Func<Task<Result<TResult>>> factory, int iterations, TimeSpan delay)
    {
        for (int i = 0; i < iterations; i++)
        {
            try
            {
                return await factory().ConfigureAwait(false);
            }
            catch
            {
                if (i == iterations - 1)
                    throw;
                await Task.Delay(delay).ConfigureAwait(false);
            }
        }

        return default;
    }
}
