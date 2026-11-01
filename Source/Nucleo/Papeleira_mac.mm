#include "Papeleira.h"
#import <Foundation/Foundation.h>

namespace xestor
{
// O Lixo do Finder (NSFileManager trashItemAtURL): dá o sitio onde quedou e «Volver poñer» funciona
juce::Result botarNoLixoMac (const juce::File& f, juce::File* onde)
{
    @autoreleasepool
    {
        NSString* ruta = [NSString stringWithUTF8String: f.getFullPathName().toRawUTF8()];
        if (ruta == nil) return juce::Result::fail (juce::String::fromUTF8 ("ruta non válida"));
        NSURL* url = [NSURL fileURLWithPath: (NSString* _Nonnull) ruta];
        NSURL* resultado = nil;
        NSError* erro = nil;
        if ([[NSFileManager defaultManager] trashItemAtURL: url resultingItemURL: &resultado error: &erro])
        {
            if (onde != nullptr && resultado != nil) *onde = juce::File (juce::String::fromUTF8 ([[resultado path] UTF8String]));
            return juce::Result::ok();
        }
        juce::String t = erro != nil ? juce::String::fromUTF8 ([[erro localizedDescription] UTF8String]) : juce::String::fromUTF8 ("erro descoñecido");
        return juce::Result::fail (t);
    }
}
} // namespace xestor
