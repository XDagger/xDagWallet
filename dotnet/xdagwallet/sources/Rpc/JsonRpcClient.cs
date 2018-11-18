using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Http;
using XDagNetWallet.Rpc.Methods;
using Newtonsoft.Json;
using XDagNetWallet.Rpc.Models;
using System.IO;

namespace XDagNetWallet.Rpc
{
    public class JsonRpcClient
    {
        private static readonly string jsonRpcServerUrl = "http://xdag.us:7667/";
        
        public JsonRpcClient()
        {

        }

        public T PostMethod<T>(MethodBase method) where T : ModelBase
        {
            string response = SendPostRequest(method.ToJsonString());
            ResultResponse<T> result = JsonConvert.DeserializeObject<ResultResponse<T>>(response);

            if (result.Error != null)
            {
                throw new JsonRpcException(result.Error.Code, result.Error.Message);
            }

            return result.Result;
        }

        private string SendPostRequest(string requstJsonBody)
        {

            string contentstring = "{\"method\":\"xdag_version\",\"id\":1,\"params\":[]}";

            HttpClient client = new HttpClient();
            client.DefaultRequestHeaders.Accept.Add(new System.Net.Http.Headers.MediaTypeWithQualityHeaderValue("application/json"));
            client.DefaultRequestHeaders.TryAddWithoutValidation("Content-Type", "application/json; charset=utf-8");

            //content.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");
            //content.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("charset=utf-8");
            //content.Headers.ContentType.Parameters.Add(new System.Net.Http.Headers.NameValueHeaderValue("IEEE754Compatible", "true"));

            HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, jsonRpcServerUrl);
            StringContent content = new StringContent(contentstring, Encoding.UTF8, "application/json");
            request.Content = content;

            // No BOM now
            StringContent content2 = new StringContent(contentstring, new UTF8Encoding(true, true), "application/json");
            StringContent content3 = new StringContent(contentstring, new UTF8Encoding(false, true), "application/json");
            request.Content = content2;


            ////content.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");
            //request.Headers.TryAddWithoutValidation("Host", "xdag.us:7667");
            //request.Headers.TryAddWithoutValidation("Expect", "100-continue");

            var task = Task.Run(async () =>
            {
                //var response = await client.PostAsync(jsonRpcServerUrl, content);

                var response = await client.SendAsync(request);
                var responseString = await response.Content.ReadAsStringAsync();
                return responseString;
            });

            task.Wait();

            return task.Result;
        }

    }
}
