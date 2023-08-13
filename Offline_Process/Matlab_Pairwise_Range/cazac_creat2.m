% function [ cazac_seq ] = cazac_creat2( signal_length, k)
% %cazac_creat 生成cazac序列
% %   signal_length:生成cazac信号长度
% %   cazac_seq:生成的cazac序列
% % k=1;%signal_length-1;
% n=1:signal_length;
% cazac_seq = zeros(signal_length, 1);
% if mod(signal_length,2)==0
%     for j=n
%         cazac_seq(j)=exp(-1i*pi*2*k/signal_length*j*j);
%     end
% else
%     for j=n
%         cazac_seq(j)=exp(-1i*pi*2*k/signal_length*j*(j+1));
%     end
% end
% end

% 
function [ cazac_seq ] = cazac_creat2( signal_length, k)
%cazac_creat 生成cazac序列
%   signal_length:生成cazac信号长度
%   cazac_seq:生成的cazac序列
% k=1;%signal_length-1;
n=1:signal_length;
if mod(signal_length,2)==0
    cazac_seq=exp(1i*pi*2*k/signal_length*(n+n.*n/2));
else
    cazac_seq=exp(1i*pi*2*k/signal_length*(n.*(n+1)/2+n));
%     cazac_seq=exp(-1i*pi*2*k/signal_length*(n.*(n+1)/2+n));
end
end