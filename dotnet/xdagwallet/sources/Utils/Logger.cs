using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Utils
{
    /// <summary>
    /// 
    /// </summary>
    public class Logger
    {
        public enum LogLevel
        {
            Trace,
            Information,
            Warning,
            Error,
            Fatal,
        }

        /// <summary>
        /// 
        /// </summary>
        private static Logger instance = null;

        public static Logger GetInstance()
        {
            if (instance == null)
            {
                instance = new Logger();
            }

            return instance;
        }

        public void WriteLog(LogLevel level, int eventId, string message)
        {
            string formattedMessage = string.Format("[{0}]{1}", eventId, message);

            log4net.ILog log = log4net.LogManager.GetLogger(Logger.RetrieveCallerMethod());
            switch (level)
            {
                case LogLevel.Trace:
                    // Trace
                    log.Debug(formattedMessage);
                    break;
                case LogLevel.Error:
                    // Error
                    log.Error(formattedMessage);
                    break;
                case LogLevel.Warning:
                    // Warning
                    log.Warn(formattedMessage);
                    break;
                case LogLevel.Information:
                    // Information
                    log.Info(formattedMessage);
                    break;
                case LogLevel.Fatal:
                    // Information
                    log.Fatal(formattedMessage);
                    break;
                default:
                    log.Fatal(formattedMessage);
                    break;
            }
        }

        public void Trace(string message)
        {
            WriteLog(LogLevel.Trace, 0, message);
        }

        public void Warning(string message)
        {
            WriteLog(LogLevel.Warning, 0, message);
        }

        public void Error(string message)
        {
            WriteLog(LogLevel.Error, 0, message);
        }

        public void Information(string message)
        {
            WriteLog(LogLevel.Information, 0, message);
        }

        private static string RetrieveCallerMethod()
        {
            int stackTraceIndex = 1;
            string fullMethodName = string.Empty;

            while (stackTraceIndex < 256)
            {
                try
                {
                    StackFrame frm = new StackFrame(stackTraceIndex);
                    MethodInfo mi = (MethodInfo)frm.GetMethod();
                    if (mi.DeclaringType != System.Reflection.MethodBase.GetCurrentMethod().DeclaringType
                         && mi.Name != "WriteLog")
                    {
                        string methodSignature = mi.ToString();
                        methodSignature = methodSignature.Substring(methodSignature.IndexOf(' ') + 1);
                        fullMethodName = string.Format("{0}.{1}", mi.DeclaringType, methodSignature);
                        break;
                    }

                    stackTraceIndex++;
                }
                catch (Exception)
                {
                    // Just break
                    break;
                }
            }

            return fullMethodName;
        }
    }
}
