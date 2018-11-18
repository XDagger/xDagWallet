using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.ExplorerAPI.Models;

namespace XDagNetWallet.ExplorerAPI
{
    public class XDagExplorerClient
    {
        private static readonly string explorerServerUrl = @"https://explorer.xdag.io/";

        public GetBlockResponse GetBlock(string address)
        {
            string responseString = SendRequest(string.Format("api/block/{0}", address));
            
            GetBlockResponse response = JsonConvert.DeserializeObject<GetBlockResponse>(responseString);

            return response;
        }

        private string SendRequest(string requestParams)
        {
            HttpClient client = new HttpClient();

            string requestUrl = string.Format("{0}{1}", explorerServerUrl, requestParams);
            HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Get, requestUrl);

            var task = Task.Run(async () =>
            {
                var response = await client.SendAsync(request);
                var responseString = await response.Content.ReadAsStringAsync();

                return responseString;
            });
            task.Wait();

            string responseStr = task.Result;
            ErrorData errorData = JsonConvert.DeserializeObject<ErrorData>(responseStr);
            if (!string.IsNullOrEmpty(errorData.ErrorCode))
            {
                throw new XDagExplorerException(errorData);
            }

            return responseStr;
        }
    }
}
