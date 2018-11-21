using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.UI
{
    /// <summary>
    /// Trigger an action after the specified TimeSpan has meet, or could be triggered manually
    /// </summary>
    public class RefreshingOperation
    {
        private TimeSpan timeoutTimeSpan = TimeSpan.Zero;

        private Action mainAction = null;

        private DateTime lastTriggeredTime = DateTime.MinValue;

        private readonly object isBusyLock = new object();

        private bool isBusy = false;

        public RefreshingOperation(Action action, TimeSpan timeout)
        {
            this.mainAction = action ?? throw new ArgumentNullException("action should not be null");

            if(timeout <= TimeSpan.Zero)
            {
                throw new ArgumentException("timeout should be positive");
            }

            this.timeoutTimeSpan = timeout;
        }

        public void Refresh(bool force = false)
        {
            if (!force && lastTriggeredTime + timeoutTimeSpan > DateTime.UtcNow)
            {
                // Just ignore the refresh request
                return;
            }

            lock (isBusyLock)
            { 
                if (isBusy)
                {
                    // Just return without doing anything
                    return;
                }

                isBusy = true;

                this.mainAction();
                lastTriggeredTime = DateTime.UtcNow;

                isBusy = false;
            }
        }

        public void ResetTime()
        {
            lastTriggeredTime = DateTime.MinValue;
        }
    }
}
